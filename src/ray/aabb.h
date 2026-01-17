// AABB: Axis-Aligned Bounding Box

#pragma once

#include "interval.h"
#include "ray.h"
#include "vec.h"

#include <cassert>

class BoundingBox {
public:
    Interval x, y, z;
    BoundingBox() = default;

    BoundingBox(const Interval& x, const Interval& y, const Interval& z) : x(x), y(y), z(z) {}

    BoundingBox(const Point3& a, const Point3& b) {
        x = Interval(std::fmin(a.x(), b.x()), std::fmax(a.x(), b.x()));
        y = Interval(std::fmin(a.y(), b.y()), std::fmax(a.y(), b.y()));
        z = Interval(std::fmin(a.z(), b.z()), std::fmax(a.z(), b.z()));
    }

    BoundingBox(const BoundingBox& box0, const BoundingBox& box1) {
        x = Interval::combine(box0.x, box1.x);
        y = Interval::combine(box0.y, box1.y);
        z = Interval::combine(box0.z, box1.z);
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
};
