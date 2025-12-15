#pragma once

#include <fmt/format.h>
#include <memory>
#include <mystl/observer-ptr.h>
#include <ranges>

namespace mystl::fmt {

struct IndentState {
    int level = 0;
};

inline thread_local IndentState fmt_indent;

inline std::string indent(int level) { return std::string(level * 2, ' '); }

struct IndentGuard {
    [[nodiscard]] IndentGuard() { ++fmt_indent.level; }
    ~IndentGuard() { --fmt_indent.level; }
    auto current() { return mystl::fmt::indent(fmt_indent.level); }
    auto base() { return mystl::fmt::indent(fmt_indent.level - 1); }
};

}  // namespace mystl::fmt

namespace fmt {

template <typename T> struct formatter<std::unique_ptr<T>> : formatter<T> {
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

struct ElementwiseFormatter {
    bool debug{false};  // debug printing, verbose and enables multi-line output
    constexpr auto parse(fmt::format_parse_context& ctx) {
        auto it = ctx.begin();
        auto end = ctx.end();
        auto start = it;
        while (it != end && *it != '}') {
            if (*it == '?') {
                debug = true;
            }
            ++it;
        }
        elem_spec = std::string_view(&*start, it - start);
        return it;
    }
    auto placeholder() const { return elem_spec.size() ? fmt::format("{{:{}}}", elem_spec) : "{}"; }

private:
    std::string_view elem_spec;
};

template <typename Pair>
    requires fmt::formattable<typename Pair::first_type> &&
             fmt::formattable<typename Pair::second_type>
struct formatter<Pair> : ElementwiseFormatter {
    constexpr auto parse(fmt::format_parse_context& ctx) { return ctx.begin(); }
    template <typename FormatContext> auto format(const Pair& p, FormatContext& ctx) const {
        mystl::fmt::IndentGuard indent;
        if (!debug) {
            return fmt::format_to(ctx.out(), "({}, {})", p.first, p.second);
        } else {
            auto fmtstr = fmt::format(
                "(\n{0}{1},\n{0}{1},\n{2})", indent.current(), placeholder(), indent.base());
            return fmt::format_to(ctx.out(), fmt::runtime(fmtstr), p.first, p.second);
        }
    }
};

template <std::ranges::range Range>
    requires(!fmt::formattable<Range>)
struct formatter<Range> : ElementwiseFormatter {
    template <typename FormatContext> auto format(const Range& range, FormatContext& ctx) const {
        mystl::fmt::IndentGuard indent;
        auto out = ctx.out();
        auto newline = fmt::format("\n{}", indent.current());
        auto newbaseline = fmt::format("\n{}", indent.base());
        out = fmt::format_to(out, "[{}", debug ? newline : "");
        bool first = true;
        for (const auto& elem : range) {
            if (!first) {
                out = fmt::format_to(out, ",{}", debug ? newline : " ");
            }
            first = false;
            out = fmt::format_to(out, fmt::runtime(placeholder()), elem);
        }
        out = fmt::format_to(out, "{}]", debug ? newbaseline : "");
        return out;
    }
};

}  // namespace fmt
