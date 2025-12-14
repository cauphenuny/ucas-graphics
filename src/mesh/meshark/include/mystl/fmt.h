#pragma once

#include <fmt/format.h>
#include <mystl/observer-ptr.h>
#include <memory>
#include <ranges>

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

template <std::ranges::range R>
requires (!fmt::formattable<R>)
struct formatter<R> {
    std::string_view elem_spec;
    bool multiline{false};
    constexpr auto parse(fmt::format_parse_context& ctx) {
        auto it = ctx.begin();
        auto end = ctx.end();
        auto start = it;
        while (it != end && *it != '}') {
            if (*it == '?') {
                multiline = true;
            }
            ++it;
        }
        elem_spec = std::string_view(&*start, it - start);
        return it;
    }
    template <typename FormatContext>
    auto format(const R& range, FormatContext& ctx) const {
        auto out = ctx.out();
        out = fmt::format_to(out, "[{}", multiline ? "\n" : "");
        bool first = true;
        auto fmtstr = elem_spec.size() ? fmt::format("{{:{}}}", elem_spec) : "{}";
        if (multiline) fmtstr = "  " + fmtstr;
        for (const auto& elem : range) {
            if (!first) {
                out = fmt::format_to(out, ",{}", multiline ? "\n" : " ");
            }
            first = false;
            out = fmt::format_to(out, fmt::runtime(fmtstr), elem);
        }
        if (multiline) out = fmt::format_to(out, ",\n");
        out = fmt::format_to(out, "]");
        return out;
    }
};

}  // namespace fmt
