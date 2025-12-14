//
// Created by creeper on 8/2/24.
//

#ifndef MESHSIMPLIFICATION_MESHARK_INCLUDE_MESHARK_MESH_ELEMENTS_H_
#define MESHSIMPLIFICATION_MESHARK_INCLUDE_MESHARK_MESH_ELEMENTS_H_

#include <cassert>
#include <fmt/format.h>
#include <meshark/element-decl.h>
#include <meshark/element-set.h>
#include <mystl/fmt.h>
#include <mystl/observer-ptr.h>
#include <string>

namespace meshark {

namespace traits {

template <typename Derived> struct StaticPolymorphism {
    const Derived& derived() const { return *static_cast<const Derived*>(this); }
    Derived& derived() { return *static_cast<Derived*>(this); }
};

struct Index {
    explicit Index(int index) : index(index) {}
    int getIndex() const { return index; }

protected:
    int index;
};

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

template <typename Type> struct ToString : StaticPolymorphism<Type> {
    [[gnu::used, gnu::noinline, gnu::visibility("default")]]  // for using in gdb/lldb
    std::string toString(bool verbose = true) const {
        static_assert(fmt::formattable<Type>, "type must be formattable by fmt");
        if constexpr (requires { fmt::format("{:?}", this->derived()); }) {
            if (verbose) {
                return fmt::format("{:?}", this->derived());
            }
        }
        return fmt::format("{}", this->derived());
    }
};

}  // namespace traits

struct HalfEdgeElement : traits::Index, traits::ToString<HalfEdgeElement> {
    explicit HalfEdgeElement(int index) : Index(index) {}
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

struct EdgeElement : traits::Index, traits::ToString<EdgeElement> {
    explicit EdgeElement(int index) : Index(index) {}

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

struct FaceElement : traits::Index, traits::ToString<FaceElement> {
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
    explicit FaceElement(int index) : Index(index) {}

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

struct VertexElement : traits::Index, traits::ToString<VertexElement> {
private:
    struct OutgoingHalfEdgeIterator {
        // for std::iterator_traits
        // in std::iter_difference_t
        // in std::sized_sentinel_for
        // in auto detecting subrange kind(sized or unsized)
        // in std::ranges::subrange template parameter deduction
        using difference_type = std::ptrdiff_t;

        OutgoingHalfEdgeIterator& operator++() {
            // TODO: implement operator++ for OutgoingHalfEdgeRange::Iterator
            it = it->twin->next;
            if (it == start) it = static_cast<HalfEdge>(nullptr);
            return *this;
        }
        OutgoingHalfEdgeIterator operator++(int) {
            OutgoingHalfEdgeIterator temp = *this;
            ++(*this);
            return temp;
        }

        HalfEdge operator*() const { return it; }

        bool operator==(const OutgoingHalfEdgeIterator& other) const { return it == other.it; }

        HalfEdge start{nullptr};
        HalfEdge it{nullptr};
    };

public:
    explicit VertexElement(int index) : Index(index) {}

    [[nodiscard]] HalfEdge halfEdge() const { return he; }

    HalfEdge& halfEdge() { return he; }

    [[nodiscard]] auto outgoingHalfEdges() const {
        return std::ranges::subrange<OutgoingHalfEdgeIterator>{
            OutgoingHalfEdgeIterator{he, he}, OutgoingHalfEdgeIterator{he, HalfEdge{nullptr}}};
    }

    void showOutgoingHalfEdges();

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

struct ElementFormattingParser {
    bool debug{false};
    bool show_addr{false};
    constexpr auto parse(format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it == '?') debug = true, ++it;
        if (it != ctx.end() && *it == 'd') show_addr = true, ++it;
        return it;
    }
    template <meshark::traits::indexable T> auto id(const T& elem) const -> std::string {
        if (show_addr) {
            if constexpr (meshark::traits::indexable_member<T>) {
                return fmt::format("{}", fmt::ptr(&elem));
            } else {
                return fmt::format("{}", fmt::ptr(elem.get()));
            }
        }
        if constexpr (meshark::traits::indexable_member<T>) {
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
            return fmt::format_to(
                ctx.out(), "HalfEdge(id={}, tail={}, tip={}, twin=HalfEdge({}))", id(he), he.tail,
                he.tip, he.twin->getIndex());
        }
        return fmt::format_to(ctx.out(), "HalfEdge({})", he.getIndex());
    }
};

template <> struct formatter<meshark::EdgeElement> : ElementFormattingParser {
    template <typename FormatContext>
    auto format(const meshark::EdgeElement& e, FormatContext& ctx) const {
        if (debug) {
            return fmt::format_to(
                ctx.out(), "Edge(id={}, v1={}, v2={})", id(e), e.firstVertex(), e.secondVertex());
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
