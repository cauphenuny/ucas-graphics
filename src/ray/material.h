#pragma once

#include "hittable.h"
#include "pdf.h"
#include "ray.h"
#include "spectrum.h"
#include "texture.h"

struct ScatterResult {
    double attenuation = 0.0;
    std::variant<std::unique_ptr<Vec3PDF>, Ray> scattered;
};

class Material {
public:
    virtual ~Material() = default;
    virtual bool scatter(const Ray& r_in, const HitResult& hit, ScatterResult& result) const {
        return false;
    }
    virtual double emit(const Ray& r_in, const HitResult& hit) const { return 0.0; }
    virtual double
    scattering_pdf(const Ray& r_in, const HitResult& hit, const Ray& scattered) const {
        return 1.0;
    }
};

class Lambertian : public Material, public traits::CreateShared<Lambertian> {
    std::shared_ptr<Texture> tex;

public:
    Lambertian(const Color& albedo) : tex(std::make_shared<ColorTexture>(albedo)) {}
    Lambertian(const std::shared_ptr<Texture>& texture) : tex(texture) {}

    bool scatter(const Ray& r_in, const HitResult& hit, ScatterResult& result) const override {
        result.scattered = std::make_unique<CosinePDF>(hit.normal);
        result.attenuation = tex->value(hit.u, hit.v, hit.p).value(r_in.wavelength());
        return result.attenuation > 0.0;
    }

    double
    scattering_pdf(const Ray& r_in, const HitResult& hit, const Ray& scattered) const override {
        auto cosine = dot(hit.normal, scattered.direction().normalized());
        return (cosine < 0) ? 0.0 : (cosine / pi);
    }
};

class Metal : public Material, public traits::CreateShared<Metal> {
    Spectrum albedo;
    double fuzz;

public:
    Metal(const Color& color, double fuzz = 0) : albedo(color), fuzz(fuzz) {}
    bool scatter(const Ray& r_in, const HitResult& hit, ScatterResult& result) const override {
        Vec3 reflected = reflect(r_in.direction().normalized(), hit.normal).normalized() +
                         (fuzz * Vec3::random_unit());
        result.scattered = r_in.redirect(hit.p, reflected);
        result.attenuation = albedo.value(r_in.wavelength());
        return dot(reflected, hit.normal) > 0;
    }
};

class Dielectric : public Material, public traits::CreateShared<Dielectric> {
    double base_index;  // Base refractive index (A in Cauchy equation)
    double dispersion;  // Dispersion coefficient (B in Cauchy equation, in μm²)

public:
    // Constructor with optional dispersion (Cauchy equation: n = A + B/λ²)
    // For glass: typical A ≈ 1.5, B ≈ 0.004-0.01 μm²
    // For diamond: A ≈ 2.4, B ≈ 0.01-0.02 μm²
    Dielectric(double ri, double dispersion_coeff = 0.3)
        : base_index(ri), dispersion(dispersion_coeff) {}

    // Calculate refractive index at given wavelength using Cauchy's equation
    double refractive_index(double wavelength_nm) const {
        if (dispersion == 0.0) {
            return base_index;
        }
        // Convert wavelength from nm to μm for Cauchy equation
        double lambda_um = wavelength_nm / 1000.0;
        return base_index + dispersion / (lambda_um * lambda_um);
    }

    bool scatter(const Ray& r_in, const HitResult& hit, ScatterResult& result) const override {
        result.attenuation = 1.0;
        double ri = refractive_index(r_in.wavelength());
        // std::cerr << "Wavelength: " << r_in.wavelength() << " nm, Refractive Index: " << ri
        //           << std::endl;
        double etai_over_etat = hit.front_face ? (1.0 / ri) : ri;

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

        result.scattered = r_in.redirect(hit.p, direction);
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

    double emit(const Ray& r_in, const HitResult& hit) const override {
        if (!hit.front_face) {
            return 0.0;
        }
        return tex->value(hit.u, hit.v, hit.p).value(r_in.wavelength()) * 1.0;
    }
};

class Isotropic : public Material, public traits::CreateShared<Isotropic> {
    std::shared_ptr<Texture> tex;

public:
    Isotropic(std::shared_ptr<Texture> tex) : tex(tex) {}
    Isotropic(const Color& color) : tex(std::make_shared<ColorTexture>(color)) {}

    bool scatter(const Ray& r_in, const HitResult& hit, ScatterResult& result) const override {
        result.scattered = std::make_unique<SpherePDF>();
        result.attenuation = tex->value(hit.u, hit.v, hit.p).value(r_in.wavelength());
        return result.attenuation > 0.0;
    }

    double
    scattering_pdf(const Ray& r_in, const HitResult& hit, const Ray& scattered) const override {
        return 1 / (4 * pi);
    }
};
