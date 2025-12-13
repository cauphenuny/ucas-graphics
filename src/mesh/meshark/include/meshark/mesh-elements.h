//
// Created by creeper on 8/2/24.
//

#ifndef MESHSIMPLIFICATION_MESHARK_INCLUDE_MESHARK_MESH_ELEMENTS_H_
#define MESHSIMPLIFICATION_MESHARK_INCLUDE_MESHARK_MESH_ELEMENTS_H_

#include <cassert>
#include <meshark/element-decl.h>
#include <meshark/element-set.h>

namespace meshark {

struct IndexedElement {
    explicit IndexedElement(int index) : index(index) {}
    int getIndex() const { return index; }

protected:
    int index;
};

struct HalfEdgeElement : IndexedElement {
    explicit HalfEdgeElement(int index) : IndexedElement(index) {}
    Vertex tip;
    Vertex tail;
    HalfEdge next;
    HalfEdge twin;
    Face face;
    Edge edge;

protected:
    template <typename Derived> friend struct HalfEdgeMesh;
};

inline HalfEdge nullHalfEdge() { return mystl::make_observer<HalfEdgeElement>(nullptr); }

struct EdgeElement : IndexedElement {
    explicit EdgeElement(int index) : IndexedElement(index) {}

    [[nodiscard]] HalfEdge halfEdge() const { return he; }

    HalfEdge& halfEdge() { return he; }

    [[nodiscard]] Vertex firstVertex() const { return he->tip; }

    [[nodiscard]] Vertex secondVertex() const { return he->tail; }

protected:
    template <typename Derived> friend struct HalfEdgeMesh;
    template <typename T> friend struct EdgeData;
    HalfEdge he;
};

inline Edge nullEdge() { return mystl::make_observer<EdgeElement>(nullptr); }

struct FaceElement : IndexedElement {
protected:
    struct BoundaryLoop {
        explicit BoundaryLoop(HalfEdge start) : start(start) {}

        struct Iterator {
            Iterator& operator++() {
                it = it->next;
                if (it == start) it = static_cast<HalfEdge>(nullptr);
                return *this;
            }

            HalfEdge operator*() const { return it; }

            bool operator==(const Iterator& other) const { return it == other.it; }

            HalfEdge start;
            HalfEdge it;
        };

        HalfEdge start;

        [[nodiscard]] Iterator begin() const {
            return {
                .start = start,
                .it = start,
            };
        }

        [[nodiscard]] Iterator end() const {
            return {.start = start, .it = static_cast<HalfEdge>(nullptr)};
        }
    };

public:
    explicit FaceElement(int index) : IndexedElement(index) {}

    [[nodiscard]] HalfEdge halfEdge() const { return he; }

    HalfEdge& halfEdge() { return he; }

    [[nodiscard]] BoundaryLoop boundaryHalfEdges() const { return BoundaryLoop(he); }

    [[nodiscard]] Vertex vertex() const { return he->tip; }

    [[nodiscard]] Edge edge() const { return he->edge; }

protected:
    template <typename Derived> friend struct HalfEdgeMesh;
    template <typename T> friend struct FaceData;
    HalfEdge he;
};
inline Face nullFace() { return mystl::make_observer<FaceElement>(nullptr); }
struct VertexElement : IndexedElement {
private:
    struct OutgoingHalfEdgeRange {
        explicit OutgoingHalfEdgeRange(HalfEdge start) : start(start) {}

        struct Iterator {
            Iterator& operator++() {
                // TODO: implement operator++ for OutgoingHalfEdgeRange::Iterator

                return *this;
            }

            HalfEdge operator*() const { return it; }

            bool operator==(const Iterator& other) const { return it == other.it; }

            HalfEdge start;
            HalfEdge it;
        };

        HalfEdge start;

        [[nodiscard]] Iterator begin() const {
            return {
                .start = start,
                .it = start,
            };
        }

        [[nodiscard]] Iterator end() const {
            return {.start = start, .it = static_cast<HalfEdge>(nullptr)};
        }
    };

public:
    explicit VertexElement(int index) : IndexedElement(index) {}

    [[nodiscard]] HalfEdge halfEdge() const { return he; }

    HalfEdge& halfEdge() { return he; }

    [[nodiscard]] OutgoingHalfEdgeRange outgoingHalfEdges() const {
        return OutgoingHalfEdgeRange(he);
    }

    [[nodiscard]] int degree() const {
        int deg = 0;
        for (auto h : outgoingHalfEdges()) deg++;
        return deg;
    }

    [[nodiscard]] VertexSet adjacentVertices() const {
        std::vector<Vertex> adj_vertices;
        adj_vertices.reserve(degree());
        for (auto h : outgoingHalfEdges()) adj_vertices.emplace_back(h->tip);
        return VertexSet(adj_vertices);
    }

    [[nodiscard]] std::optional<HalfEdge> halfEdgeTo(Vertex v) const {
        for (auto h : outgoingHalfEdges()) {
            if (h->tip == v) return h;
        }
        return std::nullopt;
    }

protected:
    template <typename Derived> friend struct HalfEdgeMesh;
    template <typename T> friend struct VertexData;
    HalfEdge he;
};
inline Vertex nullVertex() { return mystl::make_observer<VertexElement>(nullptr); }
}  // namespace meshark

namespace fmt {

struct DebugFormatter {
    bool debug{false};
    constexpr auto parse(format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it == '?') {
            debug = true;
            ++it;
        }
        return it;
    }
};

template <> struct formatter<meshark::HalfEdgeElement> : DebugFormatter {
    template <typename FormatContext>
    auto format(const meshark::HalfEdgeElement& he, FormatContext& ctx) const {
        if (debug) {
            return fmt::format_to(
                ctx.out(), "HalfEdge(id={}, index={}, tail={}, tip={})", (void*)&he, he.getIndex(),
                he.tail ? he.tail->getIndex() : -1, he.tip ? he.tip->getIndex() : -1);
        } else {
            return fmt::format_to(ctx.out(), "HalfEdge({})", he.getIndex());
        }
    }
};

template <> struct formatter<meshark::EdgeElement> : DebugFormatter {
    template <typename FormatContext>
    auto format(const meshark::EdgeElement& e, FormatContext& ctx) const {
        if (debug) {
            return fmt::format_to(
                ctx.out(), "Edge(id={}, index={}, v1={}, v2={})", (void*)&e, e.getIndex(),
                e.firstVertex() ? e.firstVertex()->getIndex() : -1,
                e.secondVertex() ? e.secondVertex()->getIndex() : -1);
        } else {
            return fmt::format_to(ctx.out(), "Edge({})", e.getIndex());
        }
    }
};

template <> struct formatter<meshark::FaceElement> : DebugFormatter {
    template <typename FormatContext>
    auto format(const meshark::FaceElement& f, FormatContext& ctx) const {
        if (debug) {
            return fmt::format_to(
                ctx.out(), "Face(id={}, index={}, halfEdge={})", (void*)&f, f.getIndex(),
                f.halfEdge() ? f.halfEdge()->getIndex() : -1);
        } else {
            return fmt::format_to(ctx.out(), "Face({})", f.getIndex());
        }
    }
};

template <> struct formatter<meshark::VertexElement> : DebugFormatter {
    template <typename FormatContext>
    auto format(const meshark::VertexElement& v, FormatContext& ctx) const {
        if (debug) {
            return fmt::format_to(
                ctx.out(), "Vertex(id={}, index={}, degree={})", (void*)&v, v.getIndex(),
                v.degree());
        } else {
            return fmt::format_to(ctx.out(), "Vertex({})", v.getIndex());
        }
    }
};

}  // namespace fmt

#endif  // MESHSIMPLIFICATION_MESHARK_INCLUDE_MESHARK_MESH_ELEMENTS_H_
