#pragma once

#include "image.h"
#include "perlin.h"
#include "spectrum.h"
#include "vec.h"

#include <algorithm>
#include <memory>
#include <utility>

class Texture {
public:
    virtual ~Texture() = default;
    virtual Spectrum value(double u, double v, const Point3& p) const = 0;
};

class ColorTexture : public Texture, public traits::CreateShared<ColorTexture> {
    Spectrum albedo;

public:
    ColorTexture(const Spectrum& spectrum) : albedo(spectrum) {}
    ColorTexture(const Color& color) : albedo(Spectrum(color)) {}
    ColorTexture(double red, double green, double blue)
        : albedo(Spectrum(Vec3(red, green, blue))) {}

    Spectrum value(double u, double v, const Point3& p) const override { return albedo; }
};

class CheckerTexture : public Texture, public traits::CreateShared<CheckerTexture> {
    std::shared_ptr<Texture> odd;
    std::shared_ptr<Texture> even;
    double inv_scale;

public:
    CheckerTexture(double scale, std::shared_ptr<Texture> even, std::shared_ptr<Texture> odd)
        : odd(std::move(odd)), even(std::move(even)), inv_scale(1.0 / scale) {}
    CheckerTexture(double scale, const Color& even, const Color& odd)
        : odd(std::make_shared<ColorTexture>(odd)), even(std::make_shared<ColorTexture>(even)),
          inv_scale(1.0 / scale) {}

    Spectrum value(double u, double v, const Point3& p) const override {
        auto x = int(std::floor(p.x() * inv_scale));
        auto y = int(std::floor(p.y() * inv_scale));
        auto z = int(std::floor(p.z() * inv_scale));

        bool is_even = (x + y + z) % 2 == 0;

        return is_even ? even->value(u, v, p) : odd->value(u, v, p);
    }
};

class ImageTexture : public Texture, public traits::CreateShared<ImageTexture> {
    Image image;

public:
    ImageTexture(const char* filename) : image(filename) {}

    Spectrum value(double u, double v, const Point3& p) const override {
        if (image.width() == 0 || image.height() == 0) {
            return Spectrum(Color(0.0, 1.0, 1.0));
        }

        u = std::clamp(u, 0.0, 1.0);
        v = 1.0 - std::clamp(v, 0.0, 1.0);

        auto i = std::clamp(static_cast<int>(u * image.width()), 0, image.width() - 1);
        auto j = std::clamp(static_cast<int>(v * image.height()), 0, image.height() - 1);

        auto pixel = image.data(i, j);
        auto r = static_cast<double>(pixel[0]) / 255.0;
        auto g = static_cast<double>(pixel[1]) / 255.0;
        auto b = static_cast<double>(pixel[2]) / 255.0;

        return Spectrum(Color(r, g, b));
    }
};

class NoiseTexture : public Texture, public traits::CreateShared<NoiseTexture> {
    Perlin perlin;
    double scale;

public:
    NoiseTexture(double scale) : scale(scale) {}
    Spectrum value(double u, double v, const Point3& p) const override {
        auto intensity = 0.5 * (perlin.noise(p * scale) + 1);
        return Spectrum::constant(intensity);
    }
};

class MarbleTexture : public Texture, public traits::CreateShared<MarbleTexture> {
    Perlin perlin;
    double scale;
    Vec3 dir;

public:
    MarbleTexture(double scale, Vec3 direction) : scale(scale), dir(direction) {}

    Spectrum value(double u, double v, const Point3& p) const override {
        auto pattern = 0.5 * (1 + std::sin(scale * dot(p, dir) + 10 * perlin.turb(p)));
        return Spectrum::constant(pattern);
    }
};

class TurbulenceTexture : public Texture, public traits::CreateShared<TurbulenceTexture> {
    Perlin perlin;
    double scale;

public:
    TurbulenceTexture(double scale) : scale(scale) {}
    Spectrum value(double u, double v, const Point3& p) const override {
        return Spectrum::constant(perlin.turb(scale * p));
    }
};
