#pragma once

#include "image.h"
#include "perlin.h"
#include "vec.h"

#include <memory>

class Texture {
public:
    virtual ~Texture() = default;
    virtual Color value(double u, double v, const Point3& p) const = 0;
};

class SolidColor : public Texture {
    Color albedo;

public:
    SolidColor(const Color& albedo) : albedo(albedo) {}
    SolidColor(double red, double green, double blue) : albedo(Color(red, green, blue)) {}

    Color value(double u, double v, const Point3& p) const override { return albedo; }
};

class CheckerTexture : public Texture {
    std::shared_ptr<Texture> odd;
    std::shared_ptr<Texture> even;
    double inv_scale;

public:
    CheckerTexture(double scale, std::shared_ptr<Texture> even, std::shared_ptr<Texture> odd)
        : odd(odd), even(even), inv_scale(1.0 / scale) {}
    CheckerTexture(double scale, const Color& even, const Color& odd)
        : odd(std::make_shared<SolidColor>(odd)), even(std::make_shared<SolidColor>(even)),
          inv_scale(1.0 / scale) {}

    Color value(double u, double v, const Point3& p) const override {
        auto x = int(std::floor(p.x() * inv_scale));
        auto y = int(std::floor(p.y() * inv_scale));
        auto z = int(std::floor(p.z() * inv_scale));

        bool is_even = (x + y + z) % 2 == 0;

        return is_even ? even->value(u, v, p) : odd->value(u, v, p);
    }
};

class ImageTexture : public Texture {
    Image image;

public:
    ImageTexture(const char* filename) : image(filename) {}

    Color value(double u, double v, const Point3& p) const override {
        if (image.width() == 0 || image.height() == 0) {
            return Color(0.0, 1.0, 1.0);  // return cyan for debug
        }

        u = std::clamp(u, 0.0, 1.0);
        v = 1.0 - std::clamp(v, 0.0, 1.0);  // flip V to image coordinates

        auto i = std::clamp(static_cast<int>(u * image.width()), 0, image.width() - 1);
        auto j = std::clamp(static_cast<int>(v * image.height()), 0, image.height() - 1);

        auto pixel = image.data(i, j);
        auto r = static_cast<double>(pixel[0]) / 255.0;
        auto g = static_cast<double>(pixel[1]) / 255.0;
        auto b = static_cast<double>(pixel[2]) / 255.0;

        return Color(r, g, b);
    }
};

class NoiseTexture : public Texture {
    Perlin perlin;
    double scale;

public:
    NoiseTexture(double scale) : scale(scale) {}
    Color value(double u, double v, const Point3& p) const override {
        return Color(1, 1, 1) * 0.5 * (perlin.noise(p * scale) + 1);
    }
};

class MarbleTexture : public Texture {
    Perlin perlin;
    double scale;
    Vec3 dir;

public:
    MarbleTexture(double scale, Vec3 direction) : scale(scale), dir(direction) {}

    Color value(double u, double v, const Point3& p) const override {
        return Color(1, 1, 1) * 0.5 * (1 + std::sin(scale * dot(p, dir) + 10 * perlin.turb(p)));
    }
};

class TurbulenceTexture : public Texture {
    Perlin perlin;
    double scale;

public:
    TurbulenceTexture(double scale) : scale(scale) {}
    Color value(double u, double v, const Point3& p) const override {
        return Color(1, 1, 1) * perlin.turb(scale * p);
    }
};
