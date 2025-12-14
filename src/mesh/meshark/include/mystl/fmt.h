#pragma once

#include <fmt/format.h>
#include <mystl/observer-ptr.h>
#include <memory>

namespace fmt {

template <typename T>
struct formatter<std::unique_ptr<T>> : formatter<T> {
    template <typename FormatContext>
    auto format(const std::unique_ptr<T>& p, FormatContext& ctx) const {
        if (p)
            return formatter<T>::format(*p, ctx);
        else
            return fmt::format_to(ctx.out(), "nullptr");
    }
};

template <typename T> struct formatter<mystl::observer_ptr<T>> : formatter<T> {
    template <typename FormatContext>
    auto format(const mystl::observer_ptr<T>& p, FormatContext& ctx) const {
        if (p) {
            return formatter<T>::format(*p.get(), ctx);
        } else {
            return fmt::format_to(ctx.out(), "nullptr");
        }
    }
};

}  // namespace fmt