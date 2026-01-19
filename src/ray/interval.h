#pragma once
#include "utility.h"

#include <format>

struct Interval {
    double min, max;
    Interval() : min(+infinity), max(-infinity) {}
    Interval(double min, double max) : min(min), max(max) {}

    static auto combine(const Interval& a, const Interval& b) {
        return Interval(std::fmin(a.min, b.min), std::fmax(a.max, b.max));
    }
    static auto intersect(const Interval& a, const Interval& b) {
        return Interval(std::fmax(a.min, b.min), std::fmin(a.max, b.max));
    }

    double size() const { return max - min; }

    bool contains(double x) const { return min <= x && x <= max; }

    bool surrounds(double x) const { return min < x && x < max; }

    double clamp(double x) const {
        if (x < min) return min;
        if (x > max) return max;
        return x;
    }

    Interval expand(double delta) const {
        auto padding = delta / 2.0;
        return Interval(min - padding, max + padding);
    }

    static const Interval empty() { return Interval(+infinity, -infinity); }
    static const Interval universe() { return Interval(-infinity, +infinity); }
};

inline Interval operator+(const Interval& a, double offset) {
    return Interval(a.min + offset, a.max + offset);
}
inline Interval operator-(const Interval& a, double offset) { return a + (-offset); }
inline Interval operator+(double offset, const Interval& a) { return a + offset; }

template <typename CharT> struct std::formatter<Interval, CharT> {
    std::formatter<double, CharT> component_formatter;

    constexpr auto parse(std::basic_format_parse_context<CharT>& ctx) {
        return component_formatter.parse(ctx);
    }

    template <typename FormatContext>
    auto format(const Interval& interval, FormatContext& ctx) const {
        auto it = ctx.out();
        it = std::format_to(it, "(");
        ctx.advance_to(it);
        it = component_formatter.format(interval.min, ctx);
        ctx.advance_to(it);
        it = std::format_to(it, ", ");
        ctx.advance_to(it);
        it = component_formatter.format(interval.max, ctx);
        ctx.advance_to(it);
        it = std::format_to(it, ")");
        ctx.advance_to(it);
        return it;
    }
};
