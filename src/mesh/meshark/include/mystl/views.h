#pragma once

#include <range/v3/all.hpp>

namespace mystl::views {

// clang-format off
struct circular_adjacent_fn {
    template <typename R>
    auto operator()(R&& r) const {
        return ranges::views::concat(
                   std::forward<R>(r), ranges::views::single(*ranges::begin(r))
               )
             | ranges::views::sliding(2)
             | ranges::views::transform([](auto&& w) {
                   auto it = w.begin();
                   return std::pair{
                       *it,
                       *std::next(it)
                   };
               });
    }
};

inline constexpr ranges::views::view_closure<circular_adjacent_fn> circular_adjacent{};
// clang-format on

}  // namespace mystl::views