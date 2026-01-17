#pragma once

#include "vec.h"

class Ray {
    Point3 orig;
    Vec3 dir;
    double tm;

public:
    Ray() = default;
    Ray(const Point3& origin, const Vec3& direction, double time = 0)
        : orig(origin), dir(direction), tm(time) {}

    const Point3& origin() const { return orig; }
    const Vec3& direction() const { return dir; }
    Point3 at(double t) const { return orig + dir * t; }
    double time() const { return tm; }
};
