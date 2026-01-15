#pragma once

#include "hittable.h"
#include "ray.h"

class Material {
public:
    virtual ~Material() = default;
    virtual bool
    scatter(const Ray& r_in, const HitResult& hit, Color& attenuation, Ray& scattered) const = 0;
};

class Lambertian : public Material {
    Color albedo;

public:
    Lambertian(const Color& albedo) : albedo(albedo) {}
    bool scatter(
        const Ray& r_in, const HitResult& hit, Color& attenuation, Ray& scattered) const override {
        auto scatter_direction = hit.normal + Vec3::random_unit();
        if (scatter_direction.near_zero()) {
            scatter_direction = hit.normal;
        }
        scattered = Ray(hit.p, scatter_direction);
        attenuation = albedo;
        return true;
    }
};

class Metal : public Material {
    Color albedo;
    double fuzz;

public:
    Metal(const Color& albedo, double fuzz = 0) : albedo(albedo), fuzz(fuzz) {}
    bool scatter(
        const Ray& r_in, const HitResult& hit, Color& attenuation, Ray& scattered) const override {
        Vec3 reflected = reflect(r_in.direction().normalized(), hit.normal).normalized() +
                         (fuzz * Vec3::random_unit());
        scattered = Ray(hit.p, reflected);
        attenuation = albedo;
        return dot(scattered.direction(), hit.normal) > 0;
    }
};
