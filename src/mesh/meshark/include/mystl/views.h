#pragma once

#include <cstddef>
#include <iterator>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <utility>

namespace mystl::views {

template <std::ranges::view V>
class enumerate_view : public std::ranges::view_interface<enumerate_view<V>> {
    V base_;

    template <bool Const> class iterator {
        using Base = std::conditional_t<Const, const V, V>;
        std::ranges::iterator_t<Base> current_;
        std::size_t index_;

    public:
        using iterator_category =
            typename std::iterator_traits<std::ranges::iterator_t<Base>>::iterator_category;
        using difference_type = std::ptrdiff_t;

        iterator() = default;

        iterator(std::ranges::iterator_t<Base> it, std::size_t idx) : current_(it), index_(idx) {}

        auto operator*() const {
            return std::pair<std::size_t, std::ranges::range_reference_t<Base>>{index_, *current_};
        }

        iterator& operator++() {
            ++current_;
            ++index_;
            return *this;
        }

        void operator++(int) { ++*this; }

        friend bool operator==(const iterator& x, const iterator& y)
            requires std::equality_comparable<std::ranges::iterator_t<Base>>
        {
            return x.current_ == y.current_;
        }
    };

public:
    enumerate_view() = default;
    explicit enumerate_view(V base) : base_(std::move(base)) {}

    auto begin() { return iterator<false>(std::ranges::begin(base_), 0); }

    auto end() { return iterator<false>(std::ranges::end(base_), 0); }

    auto begin() const
        requires std::ranges::range<const V>
    {
        return iterator<true>(std::ranges::begin(base_), 0);
    }

    auto end() const
        requires std::ranges::range<const V>
    {
        return iterator<true>(std::ranges::end(base_), 0);
    }
};

inline constexpr struct {
    template <std::ranges::viewable_range R> auto operator()(R&& r) const {
        return enumerate_view<std::views::all_t<R>>(std::views::all(std::forward<R>(r)));
    }
} enumerate;

template <typename... Ts> using tuple = std::tuple<Ts...>;

template <typename F, typename Tuple, std::size_t... Is>
constexpr decltype(auto) tuple_transform_impl(F&& f, Tuple&& t, std::index_sequence<Is...>) {
    return tuple{f(std::get<Is>(std::forward<Tuple>(t)))...};
}

template <typename F, typename Tuple> constexpr decltype(auto) tuple_transform(F&& f, Tuple&& t) {
    constexpr std::size_t N = std::tuple_size_v<std::remove_reference_t<Tuple>>;
    return tuple_transform_impl(
        std::forward<F>(f), std::forward<Tuple>(t), std::make_index_sequence<N>{});
}

template <typename Tuple> using tuple_decay_t = std::remove_reference_t<Tuple>;

template <typename Tuple>
using tuple_indices = std::make_index_sequence<std::tuple_size_v<tuple_decay_t<Tuple>>>;

template <typename Tuple, typename Seq> struct tuple_iterator_tuple_impl;

template <typename Tuple, std::size_t... Is>
struct tuple_iterator_tuple_impl<Tuple, std::index_sequence<Is...>> {
    using type = tuple<std::ranges::iterator_t<std::tuple_element_t<Is, tuple_decay_t<Tuple>>>...>;
};

template <typename Tuple>
using tuple_iterator_tuple_t =
    typename tuple_iterator_tuple_impl<Tuple, tuple_indices<Tuple>>::type;

template <typename Tuple, typename Seq> struct tuple_sentinel_tuple_impl;

template <typename Tuple, std::size_t... Is>
struct tuple_sentinel_tuple_impl<Tuple, std::index_sequence<Is...>> {
    using type = tuple<std::ranges::sentinel_t<std::tuple_element_t<Is, tuple_decay_t<Tuple>>>...>;
};

template <typename Tuple>
using tuple_sentinel_tuple_t =
    typename tuple_sentinel_tuple_impl<Tuple, tuple_indices<Tuple>>::type;

template <typename Tuple1, typename Tuple2, std::size_t... Is>
constexpr bool
tuple_any_equal_impl(const Tuple1& lhs, const Tuple2& rhs, std::index_sequence<Is...>) {
    bool finished = false;
    if constexpr (sizeof...(Is) > 0) {
        ((finished = finished || (std::get<Is>(lhs) == std::get<Is>(rhs))), ...);
    }
    return finished;
}

template <typename Tuple1, typename Tuple2>
constexpr bool tuple_any_equal(const Tuple1& lhs, const Tuple2& rhs) {
    static_assert(
        std::tuple_size_v<tuple_decay_t<Tuple1>> == std::tuple_size_v<tuple_decay_t<Tuple2>>,
        "Tuple sizes must match");
    return tuple_any_equal_impl(lhs, rhs, tuple_indices<Tuple1>{});
}

template <std::ranges::view... Views>
class zip_view : public std::ranges::view_interface<zip_view<Views...>> {
    tuple<Views...> bases_;

    template <bool Const> class iterator {
        using Bases = std::conditional_t<Const, tuple<const Views...>, tuple<Views...>>;
        using IterTuple = tuple_iterator_tuple_t<Bases>;

        IterTuple its_;

    public:
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::input_iterator_tag;

        iterator() = default;

        explicit iterator(IterTuple its) : its_(std::move(its)) {}

        auto operator*() const {
            return tuple_transform([](auto& it) -> decltype(auto) { return *it; }, its_);
        }

        iterator& operator++() {
            tuple_transform(
                [](auto& it) {
                    ++it;
                    return 0;
                },
                its_);
            return *this;
        }

        void operator++(int) { ++*this; }

        friend bool operator==(const iterator& x, const iterator& y) {
            // end condition handled by sentinel
            return x.its_ == y.its_;
        }
    };

    template <bool Const> class sentinel {
        using Bases = std::conditional_t<Const, tuple<const Views...>, tuple<Views...>>;
        using SentTuple = tuple_sentinel_tuple_t<Bases>;
        SentTuple ends_;

    public:
        sentinel() = default;

        explicit sentinel(SentTuple ends) : ends_(std::move(ends)) {}

        friend bool operator==(const iterator<Const>& it, const sentinel& s) {
            return tuple_any_equal(it.its_, s.ends_);
        }

        friend bool operator==(const sentinel& s, const iterator<Const>& it) { return it == s; }
    };

public:
    zip_view() = default;
    explicit zip_view(Views... views) : bases_(std::move(views)...) {}

    auto begin() {
        return iterator<false>(
            tuple_transform([](auto& v) { return std::ranges::begin(v); }, bases_));
    }

    auto end() {
        return sentinel<false>(
            tuple_transform([](auto& v) { return std::ranges::end(v); }, bases_));
    }

    auto begin() const
        requires(std::ranges::range<const Views> && ...)
    {
        return iterator<true>(
            tuple_transform([](auto const& v) { return std::ranges::begin(v); }, bases_));
    }

    auto end() const
        requires(std::ranges::range<const Views> && ...)
    {
        return sentinel<true>(
            tuple_transform([](auto const& v) { return std::ranges::end(v); }, bases_));
    }
};

inline constexpr struct {
    template <std::ranges::viewable_range... R> auto operator()(R&&... r) const {
        return zip_view<std::views::all_t<R>...>(std::views::all(std::forward<R>(r))...);
    }
} zip;

}  // namespace mystl::views
