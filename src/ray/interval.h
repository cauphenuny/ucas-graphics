#pragma once
#include "utility.h"

struct Interval {
    double min, max;
    Interval() : min(+infinity), max(-infinity) {}
    Interval(double min, double max) : min(min), max(max) {}

    double size() const { return max - min; }

    bool contains(double x) const { return min <= x && x <= max; }

    bool surrounds(double x) const { return min < x && x < max; }

    double clamp(double x) const {
        if (x < min) return min;
        if (x > max) return max;
        return x;
    }

    static const Interval empty() { return Interval(+infinity, -infinity); }

    static const Interval universe() { return Interval(-infinity, +infinity); }
};
