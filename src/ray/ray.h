#pragma once

#include "vec.h"

class Ray {
    Point3 orig;
    Vec3 dir;

public:
    Ray() = default;
    Ray(const Point3& origin, const Vec3& direction) : orig(origin), dir(direction) {}

    const Point3& origin() const { return orig; }
    const Vec3& direction() const { return dir; }
    Point3 at(double t) const { return orig + dir * t; }
};
