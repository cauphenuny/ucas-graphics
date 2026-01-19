#pragma once

#include "hittable.h"
#include "material.h"
#include "texture.h"

#include <memory>

class ConstantMedium : public Hittable, public traits::CreateShared<ConstantMedium> {
    std::shared_ptr<Hittable> boundary;
    std::shared_ptr<Material> phase_function;
    double neg_inv_density;

public:
    ConstantMedium(std::shared_ptr<Hittable> b, double d, std::shared_ptr<Texture> tex)
        : boundary(std::move(b)), neg_inv_density(-1.0 / d),
          phase_function(Isotropic::create(tex)) {}

    ConstantMedium(std::shared_ptr<Hittable> b, double d, const Color& color)
        : boundary(std::move(b)), neg_inv_density(-1.0 / d),
          phase_function(Isotropic::create(color)) {}

    BoundingBox bounding_box() const override { return boundary->bounding_box(); }

    bool hit(const Ray& ray, Interval interval, HitResult& result) const override {
        HitResult hit1, hit2;
        if (!boundary->hit(ray, Interval::universe(), hit1)) return false;
        if (!boundary->hit(ray, Interval(hit1.t + 1e-4, infinity), hit2)) return false;

        hit1.t = std::fmax(hit1.t, interval.min);
        hit2.t = std::fmin(hit2.t, interval.max);

        if (hit1.t >= hit2.t) return false;

        if (hit1.t < 0) hit1.t = 0;

        auto ray_length = ray.direction().norm();
        auto distance = (hit2.t - hit1.t) * ray_length;

        auto hit_distance = neg_inv_density * std::log(random_double());

        if (hit_distance > distance) return false;

        result.t = hit1.t + hit_distance / ray_length;
        result.p = ray.at(result.t);
        result.normal = Vec3(1, 0, 0);  // arbitrary
        result.front_face = true;       // also arbitrary
        result.mat = phase_function;

        return true;
    }
};
