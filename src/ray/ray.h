#pragma once

#include "vec.h"

class Ray {
    Point3 orig;
    Vec3 dir;
    double tm;
    double lambda;

public:
    static constexpr double default_wavelength = 550.0;

    Ray() = default;
    Ray(const Point3& origin, const Vec3& direction, double wavelength = default_wavelength,
        double time = 0)
        : orig(origin), dir(direction), tm(time), lambda(wavelength) {}

    Ray redirect(const Point3& new_origin, const Vec3& new_direction) const {
        return Ray(new_origin, new_direction, lambda, tm);
    }

    const Point3& origin() const { return orig; }
    const Vec3& direction() const { return dir; }
    Point3 at(double t) const { return orig + dir * t; }
    double time() const { return tm; }
    double wavelength() const { return lambda; }
};
