//
// Created by creeper on 8/2/24.
//

#ifndef MESHSIMPLIFICATION_MESHARK_INCLUDE_MESHARK_MESH_ELEMENTS_H_
#define MESHSIMPLIFICATION_MESHARK_INCLUDE_MESHARK_MESH_ELEMENTS_H_

#include <cassert>
#include <meshark/element-decl.h>
#include <meshark/element-set.h>
#include <mystl/observer-ptr.h>
#include <string>

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
                it = it->twin->next;
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

template <typename T>
concept indexable_member = requires(T a) {
    { a.getIndex() } -> std::convertible_to<int>;
};

template <typename T>
concept indexable_pointer = requires(T a) {
    { a->getIndex() } -> std::convertible_to<int>;
};

template <typename T>
concept indexable = indexable_member<T> || indexable_pointer<T>;

}  // namespace meshark

namespace fmt {

template <typename T> struct formatter<mystl::observer_ptr<T>>;

struct ElementFormattingParser {
    bool debug{false};
    bool show_addr{false};
    constexpr auto parse(format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it == '?') debug = true, ++it;
        if (it != ctx.end() && *it == 'd') show_addr = true, ++it;
        return it;
    }
    template <meshark::indexable T> auto id(const T& elem) const -> std::string {
        if (show_addr) {
            if constexpr (meshark::indexable_member<T>) {
                return fmt::format("{}", fmt::ptr(&elem));
            } else {
                return fmt::format("{}", fmt::ptr(elem.get()));
            }
        }
        if constexpr (meshark::indexable_member<T>) {
            return std::to_string(elem.getIndex());
        } else {
            return std::to_string(elem->getIndex());
        }
    }
};

template <> struct formatter<meshark::HalfEdgeElement> : ElementFormattingParser {
    template <typename FormatContext>
    auto format(const meshark::HalfEdgeElement& he, FormatContext& ctx) const {
        if (debug) {
            assert(he.twin && "twin edge broken");
            return fmt::format_to(ctx.out(), "HalfEdge(id={}, tail={}, tip={}, twin=HalfEdge({}))", id(he), he.tail, he.tip, he.twin->getIndex());
        }
        return fmt::format_to(ctx.out(), "HalfEdge({})", he.getIndex());
    }
};

template <> struct formatter<meshark::EdgeElement> : ElementFormattingParser {
    template <typename FormatContext>
    auto format(const meshark::EdgeElement& e, FormatContext& ctx) const {
        if (debug) {
            return fmt::format_to(ctx.out(), "Edge(id={}, v1={}, v2={})", id(e), e.firstVertex(), e.secondVertex());
        } else {
            return fmt::format_to(ctx.out(), "Edge({})", e.getIndex());
        }
    }
};

template <> struct formatter<meshark::FaceElement> : ElementFormattingParser {
    template <typename FormatContext>
    auto format(const meshark::FaceElement& f, FormatContext& ctx) const {
        if (debug) {
            return fmt::format_to(ctx.out(), "Face(id={}, halfEdge={})", id(f), f.halfEdge());
        } else {
            return fmt::format_to(ctx.out(), "Face({})", f.getIndex());
        }
    }
};

template <> struct formatter<meshark::VertexElement> : ElementFormattingParser {
    template <typename FormatContext>
    auto format(const meshark::VertexElement& v, FormatContext& ctx) const {
        if (debug) {
            return fmt::format_to(ctx.out(), "Vertex(id={}, degree={})", id(v), v.degree());
        } else {
            return fmt::format_to(ctx.out(), "Vertex({})", v.getIndex());
        }
    }
};

}  // namespace fmt

#endif  // MESHSIMPLIFICATION_MESHARK_INCLUDE_MESHARK_MESH_ELEMENTS_H_
