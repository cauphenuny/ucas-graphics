// AABB: Axis-Aligned Bounding Box

#pragma once

#include "interval.h"
#include "ray.h"
#include "vec.h"

#include <cassert>
#include <format>

class BoundingBox {
    void apply_pad() {
        double delta = 0.0001;
        if (x.size() < delta) x = x.expand(delta);
        if (y.size() < delta) y = y.expand(delta);
        if (z.size() < delta) z = z.expand(delta);
    }

public:
    Interval x, y, z;
    BoundingBox() = default;

    BoundingBox(const Interval& x, const Interval& y, const Interval& z) : x(x), y(y), z(z) {
        apply_pad();
    }

    static BoundingBox diag(const Point3& a, const Point3& b) {
        auto x = Interval(std::fmin(a.x(), b.x()), std::fmax(a.x(), b.x()));
        auto y = Interval(std::fmin(a.y(), b.y()), std::fmax(a.y(), b.y()));
        auto z = Interval(std::fmin(a.z(), b.z()), std::fmax(a.z(), b.z()));
        return BoundingBox(x, y, z);
    }

    static BoundingBox combine(const BoundingBox& box0, const BoundingBox& box1) {
        auto x = Interval::combine(box0.x, box1.x);
        auto y = Interval::combine(box0.y, box1.y);
        auto z = Interval::combine(box0.z, box1.z);
        return BoundingBox(x, y, z);
    }

    const Interval& axis_interval(int axis) const {
        assert(axis >= 0 && axis < 3);
        switch (axis) {
            case 1: return y;
            case 2: return z;
            default: return x;
        }
    }

    bool hit(const Ray& ray, Interval interval) const {
        const Point3& orig = ray.origin();
        const Point3& dir = ray.direction();

        for (int axis = 0; axis < 3; axis++) {
            const Interval& ax = axis_interval(axis);
            const double inv_d = 1.0 / dir[axis];

            auto t0 = (ax.min - orig[axis]) * inv_d;
            auto t1 = (ax.max - orig[axis]) * inv_d;

            if (t0 > t1) std::swap(t0, t1);

            interval.min = std::fmax(t0, interval.min);
            interval.max = std::fmin(t1, interval.max);

            if (interval.max <= interval.min) {
                return false;
            }
        }
        return true;
    }

    int longest_axis() const {
        if (x.size() > y.size()) {
            return x.size() > z.size() ? 0 : 2;
        } else {
            return y.size() > z.size() ? 1 : 2;
        }
    }

    static BoundingBox empty() {
        return BoundingBox(Interval::empty(), Interval::empty(), Interval::empty());
    }
    static BoundingBox universe() {
        return BoundingBox(Interval::universe(), Interval::universe(), Interval::universe());
    }
};

template <typename CharT> struct std::formatter<BoundingBox, CharT> {
    std::formatter<Interval, CharT> interval_formatter;

    constexpr auto parse(std::basic_format_parse_context<CharT>& ctx) {
        return interval_formatter.parse(ctx);
    }

    template <typename FormatContext> auto format(const BoundingBox& box, FormatContext& ctx) const {
        auto it = ctx.out();
        it = std::format_to(it, "(");
        ctx.advance_to(it);
        it = interval_formatter.format(box.x, ctx);
        ctx.advance_to(it);
        it = std::format_to(it, ", ");
        ctx.advance_to(it);
        it = interval_formatter.format(box.y, ctx);
        ctx.advance_to(it);
        it = std::format_to(it, ", ");
        ctx.advance_to(it);
        it = interval_formatter.format(box.z, ctx);
        ctx.advance_to(it);
        it = std::format_to(it, ")");
        ctx.advance_to(it);
        return it;
    }
};
