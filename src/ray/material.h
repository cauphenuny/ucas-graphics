#pragma once

#include "hittable.h"
#include "onb.h"
#include "ray.h"
#include "texture.h"

class Material {
public:
    virtual ~Material() = default;
    virtual bool scatter(
        const Ray& r_in, const HitResult& hit, Color& attenuation, Ray& scattered,
        double& pdf) const {
        return false;
    }
    virtual Color emit(const Ray& r_in, const HitResult& hit) const { return Color(0, 0, 0); }
    virtual double
    scattering_pdf(const Ray& r_in, const HitResult& hit, const Ray& scattered) const {
        return 0.0;
    }
};

class Lambertian : public Material, public traits::CreateShared<Lambertian> {
    std::shared_ptr<Texture> tex;

public:
    Lambertian(const Color& albedo) : tex(std::make_shared<ColorTexture>(albedo)) {}
    Lambertian(const std::shared_ptr<Texture>& texture) : tex(texture) {}

    bool scatter(
        const Ray& r_in, const HitResult& hit, Color& attenuation, Ray& scattered,
        double& pdf) const override {
        OrthonormalBasis uvw(hit.normal);
        auto scatter_direction = uvw.transform(Vec3::random_cosine_direction());
        if (scatter_direction.near_zero()) {
            scatter_direction = hit.normal;
        }
        scattered = Ray(hit.p, scatter_direction, r_in.time());
        attenuation = tex->value(hit.u, hit.v, hit.p);
        pdf = dot(uvw.w(), scattered.direction().normalized()) / pi;
        return true;
    }

    double
    scattering_pdf(const Ray& r_in, const HitResult& hit, const Ray& scattered) const override {
        auto cosine = dot(hit.normal, scattered.direction().normalized());
        return (cosine < 0) ? 0.0 : (cosine / pi);
    }
};

class Metal : public Material, public traits::CreateShared<Metal> {
    Color albedo;
    double fuzz;

public:
    Metal(const Color& albedo, double fuzz = 0) : albedo(albedo), fuzz(fuzz) {}
    bool scatter(
        const Ray& r_in, const HitResult& hit, Color& attenuation, Ray& scattered,
        double& pdf) const override {
        Vec3 reflected = reflect(r_in.direction().normalized(), hit.normal).normalized() +
                         (fuzz * Vec3::random_unit());
        scattered = Ray(hit.p, reflected, r_in.time());
        attenuation = albedo;
        return dot(scattered.direction(), hit.normal) > 0;
    }
};

class Dielectric : public Material, public traits::CreateShared<Dielectric> {
    double refraction_index;

public:
    Dielectric(double ri) : refraction_index(ri) {}

    bool scatter(
        const Ray& r_in, const HitResult& hit, Color& attenuation, Ray& scattered,
        double& pdf) const override {
        attenuation = Color(1.0, 1.0, 1.0);
        double etai_over_etat = hit.front_face ? (1.0 / refraction_index) : refraction_index;

        Vec3 unit_direction = r_in.direction().normalized();
        double cos_theta = std::fmin(dot(-unit_direction, hit.normal), 1.0);
        double sin_theta = std::sqrt(1.0 - cos_theta * cos_theta);

        bool cannot_refract = etai_over_etat * sin_theta > 1.0;
        Vec3 direction;

        if (cannot_refract || reflectance(cos_theta, etai_over_etat) > random_double()) {
            direction = reflect(unit_direction, hit.normal);
        } else {
            direction = refract(unit_direction, hit.normal, etai_over_etat);
        }

        scattered = Ray(hit.p, direction, r_in.time());
        return true;
    }

    static double reflectance(double cosine, double ref_idx) {
        // NOTE: Schlick's approximation for reflectance.
        auto r0 = (1 - ref_idx) / (1 + ref_idx);
        r0 = r0 * r0;
        return r0 + (1 - r0) * std::pow((1 - cosine), 5);
    }
};

class Light : public Material, public traits::CreateShared<Light> {
    std::shared_ptr<Texture> tex;

public:
    Light(std::shared_ptr<Texture> tex) : tex(tex) {}
    Light(const Color& color) : tex(std::make_shared<ColorTexture>(color)) {}

    Color emit(const Ray& r_in, const HitResult& hit) const override {
        if (!hit.front_face) {
            return Color(0, 0, 0);
        }
        return tex->value(hit.u, hit.v, hit.p);
    }
};

class Isotropic : public Material, public traits::CreateShared<Isotropic> {
    std::shared_ptr<Texture> tex;

public:
    Isotropic(std::shared_ptr<Texture> tex) : tex(tex) {}
    Isotropic(const Color& color) : tex(std::make_shared<ColorTexture>(color)) {}

    bool scatter(
        const Ray& r_in, const HitResult& hit, Color& attenuation, Ray& scattered,
        double& pdf) const override {
        scattered = Ray(hit.p, Vec3::random_unit(), r_in.time());
        attenuation = tex->value(hit.u, hit.v, hit.p);
        pdf = 1 / (4 * pi);
        return true;
    }

    double
    scattering_pdf(const Ray& r_in, const HitResult& hit, const Ray& scattered) const override {
        return 1 / (4 * pi);
    }
};
