#pragma once

#include "object.h"
#include "ray.h"
#include "vec.h"

class Sphere : public Object {
    double radius;
    Point3 center;

public:
    Sphere(const Point3& center, double radius) : center(center), radius(std::max(0., radius)) {}

    bool hit(const Ray& ray, Interval interval, HitResult& result) const override {
        auto oc = center - ray.origin();
        auto a = ray.direction().sqrnorm();
        auto h = dot(ray.direction(), oc);
        auto c = oc.sqrnorm() - radius * radius;
        auto discriminant = h * h - a * c;
        if (discriminant < 0) {
            return false;
        }
        auto sqrtd = std::sqrt(discriminant);

        auto root = (h - sqrtd) / a;
        if (!interval.surrounds(root)) {
            root = (h + sqrtd) / a;
            if (!interval.surrounds(root)) {
                return false;
            }
        }
        result.t = root;
        result.p = ray.at(root);
        Vec3 outward_normal = (result.p - center) / radius;
        result.set_face_normal(ray, outward_normal);
        return true;
    }
};
