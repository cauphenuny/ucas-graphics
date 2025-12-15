//
// Created by creeper on 8/2/24.
//

#ifndef MESHSIMPLIFICATION_MESHARK_INCLUDE_MESHARK_MESH_ELEMENTS_H_
#define MESHSIMPLIFICATION_MESHARK_INCLUDE_MESHARK_MESH_ELEMENTS_H_

#include <fmt/format.h>
#include <meshark/element-decl.h>
#include <meshark/element-set.h>
#include <mystl/fmt.h>
#include <mystl/observer-ptr.h>
#include <ranges>
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
    [[gnu::used, gnu::noinline, gnu::visibility("default")]]  // for using it in gdb/lldb
    std::string repr() const {                                // name from python __repr__ method
        static_assert(fmt::formattable<Type>, "type must be formattable by fmt");
        // use debug format if available
        if constexpr (requires { fmt::format("{:?}", this->derived()); }) {
            return fmt::format("{:?}", this->derived());
        } else {
            return fmt::format("{}", this->derived());
        }
    }
    [[gnu::used, gnu::noinline, gnu::visibility("default")]]
    std::string str() const {
        static_assert(fmt::formattable<Type>, "type must be formattable by fmt");
        return fmt::format("{}", this->derived());
    }
    [[gnu::used, gnu::noinline, gnu::visibility("default")]]
    operator std::string() const {
        return str();
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
    struct BoundaryIterator {
        using difference_type = std::ptrdiff_t;
        using value_type = HalfEdge;
        BoundaryIterator& operator++() {
            it = it->next;
            if (it == start) it = static_cast<HalfEdge>(nullptr);
            return *this;
        }
        BoundaryIterator operator++(int) {
            BoundaryIterator temp = *this;
            ++(*this);
            return temp;
        }

        HalfEdge operator*() const { return it; }

        bool operator==(const BoundaryIterator& other) const { return it == other.it; }

        HalfEdge start;
        HalfEdge it;
    };

public:
    explicit FaceElement(int index) : Index(index) {}

    [[nodiscard]] HalfEdge halfEdge() const { return he; }

    HalfEdge& halfEdge() { return he; }

    [[nodiscard]] auto boundaryHalfEdges() const {
        return std::ranges::subrange<BoundaryIterator>{
            BoundaryIterator{he, he}, BoundaryIterator{he, HalfEdge{nullptr}}};
    }

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
        // in ranges::subrange template parameter deduction
        using difference_type = std::ptrdiff_t;
        // for ranges::views operations
        using value_type = HalfEdge;

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
    bool no_name{false};
    bool show_addr{false};
    constexpr auto parse(format_parse_context& ctx) {
        auto it = ctx.begin();
        while (it != ctx.end()) {
            switch (*it) {
                case '?': debug = true; break;
                case '&': show_addr = true; break;
                case 'n': no_name = true; break;
                default: return it;
            }
            ++it;
        }
        return it;
    }
    template <meshark::traits::indexable T> auto id(const T& elem) const -> std::string {
        if (show_addr) {
            if constexpr (meshark::traits::indexable_member<T>) {
                return fmt::format("{} <{}>", elem.getIndex(), fmt::ptr(&elem));
            } else {
                return fmt::format("{} <{}>", elem.getIndex(), fmt::ptr(elem.get()));
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
            auto twin = he.twin ? fmt::format("{}", he.twin->getIndex()) : "nullptr";
            auto next = he.next ? fmt::format("{}", he.next->getIndex()) : "nullptr";
            return fmt::format_to(
                ctx.out(),
                "HalfEdge(id={}, tail={:n}, tip={:n}, twin={}, next={}, edge={:n}, face={:n})",
                id(he), he.tail, he.tip, twin, next, he.edge, he.face);
        }
        if (no_name) return fmt::format_to(ctx.out(), "{}", id(he));
        return fmt::format_to(ctx.out(), "HalfEdge({})", id(he));
    }
};

template <> struct formatter<meshark::EdgeElement> : ElementFormattingParser {
    template <typename FormatContext>
    auto format(const meshark::EdgeElement& e, FormatContext& ctx) const {
        if (debug) {
            return fmt::format_to(
                ctx.out(), "Edge(id={}, v1={:n}, v2={:n})", id(e), e.firstVertex(),
                e.secondVertex());
        }
        if (no_name) return fmt::format_to(ctx.out(), "{}", id(e));
        return fmt::format_to(ctx.out(), "Edge({})", id(e));
    }
};

template <> struct formatter<meshark::FaceElement> : ElementFormattingParser {
    template <typename FormatContext>
    auto format(const meshark::FaceElement& f, FormatContext& ctx) const {
        if (debug) {
            auto boundary_vertices =
                f.boundaryHalfEdges() | std::views::transform([](auto he) { return he->tail; });
            return fmt::format_to(
                ctx.out(), "Face(id={}, boundary={:n})", id(f), boundary_vertices);
        }
        if (no_name) return fmt::format_to(ctx.out(), "{}", id(f));
        return fmt::format_to(ctx.out(), "Face({})", id(f));
    }
};

template <> struct formatter<meshark::VertexElement> : ElementFormattingParser {
    template <typename FormatContext>
    auto format(const meshark::VertexElement& v, FormatContext& ctx) const {
        if (debug) {
            return fmt::format_to(ctx.out(), "Vertex(id={}, degree={})", id(v), v.degree());
        }
        if (no_name) return fmt::format_to(ctx.out(), "{}", id(v));
        return fmt::format_to(ctx.out(), "Vertex({})", id(v));
    }
};

}  // namespace fmt

#endif
