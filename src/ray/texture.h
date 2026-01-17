#pragma once

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
        : odd(std::make_shared<SolidColor>(odd)),
          even(std::make_shared<SolidColor>(even)),
          inv_scale(1.0 / scale) {}

    Color value(double u, double v, const Point3& p) const override {
        auto x = int(std::floor(p.x() * inv_scale));
        auto y = int(std::floor(p.y() * inv_scale));
        auto z = int(std::floor(p.z() * inv_scale));

        bool is_even = (x + y + z) % 2 == 0;

        return is_even ? even->value(u, v, p) : odd->value(u, v, p);
    }
};
