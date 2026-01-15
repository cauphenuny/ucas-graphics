#pragma once

#include "hittable.h"
#include "material.h"
#include "ray.h"
#include "vec.h"

#include <memory>

class Sphere : public Hittable {
    double radius;
    Point3 center;
    std::shared_ptr<Material> mat;

public:
    Sphere(const Point3& center, double radius, std::shared_ptr<Material> mat)
        : center(center), radius(std::max(0., radius)), mat(mat) {}

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
        result.mat = mat;
        return true;
    }
};
