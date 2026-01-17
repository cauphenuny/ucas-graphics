#pragma once
#include "utility.h"

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
