#pragma once

#include "object.h"
#include "ray.h"
#include "vec.h"

class Sphere : public Object {
    double radius;
    Point3 center;

public:
    Sphere(const Point3& center, double radius) : center(center), radius(std::max(0., radius)) {}

    bool hit(const Ray& r, double tmin, double tmax, HitResult& result) const override {
        auto oc = center - r.origin();
        auto a = r.direction().sqrnorm();
        auto h = dot(r.direction(), oc);
        auto c = oc.sqrnorm() - radius * radius;
        auto discriminant = h * h - a * c;
        if (discriminant < 0) {
            return false;
        }
        auto sqrtd = std::sqrt(discriminant);

        auto root = (h - sqrtd) / a;
        if (root <= tmin || root >= tmax) {
            root = (h + sqrtd) / a;
            if (root <= tmin || root >= tmax) {
                return false;
            }
        }
        result.t = root;
        result.p = r.at(root);
        Vec3 outward_normal = (result.p - center) / radius;
        result.set_face_normal(r, outward_normal);
        return true;
    }
};
