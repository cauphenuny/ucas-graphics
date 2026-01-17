#pragma once

#include "aabb.h"
#include "hittable.h"
#include "material.h"
#include "ray.h"
#include "vec.h"

#include <memory>

class Sphere : public Hittable {
    double radius;
    Ray center;
    std::shared_ptr<Material> mat;
    BoundingBox bbox;

public:
    Sphere(const Point3& center, double radius, std::shared_ptr<Material> mat)
        : Sphere(center, center, radius, mat) {}

    Sphere(
        const Point3& center_start, const Point3& center_end, double radius,
        std::shared_ptr<Material> mat)
        : center(center_start, center_end - center_start), radius(std::max(0., radius)), mat(mat) {
        auto rvec = Vec3(radius, radius, radius);
        auto box0 = BoundingBox(center.at(0) - rvec, center.at(0) + rvec);
        auto box1 = BoundingBox(center.at(1) - rvec, center.at(1) + rvec);
        bbox = BoundingBox(box0, box1);
    }

    BoundingBox bounding_box() const override { return bbox; }

    static auto sphere_uv(const Point3& p) {
        // p: a given point on the sphere of radius one, centered at the origin.
        // u: returned value [0,1] of angle around the Y axis from X=-1.
        // v: returned value [0,1] of angle from Y=-1 to Y=+1.
        //     <1 0 0> yields <0.50 0.50>       <-1  0  0> yields <0.00 0.50>
        //     <0 1 0> yields <0.50 1.00>       < 0 -1  0> yields <0.50 0.00>
        //     <0 0 1> yields <0.25 0.50>       < 0  0 -1> yields <0.75 0.50>

        auto theta = std::acos(-p.y());
        auto phi = std::atan2(-p.z(), p.x()) + pi;  // NOTE: to make result grows as z grows

        auto u = phi / (2 * pi);
        auto v = theta / pi;
        return std::make_tuple(u, v);
    }

    bool hit(const Ray& ray, Interval interval, HitResult& result) const override {
        auto current_center = center.at(ray.time());
        auto oc = current_center - ray.origin();
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
        Vec3 outward_normal = (result.p - current_center) / radius;
        std::tie(result.u, result.v) = sphere_uv(outward_normal);
        result.set_face_normal(ray, outward_normal);
        result.mat = mat;
        return true;
    }
};
