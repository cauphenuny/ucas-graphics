#pragma once

#include "hittable.h"
#include "onb.h"

class Vec3PDF {
public:
    virtual ~Vec3PDF() = default;
    virtual double value(const Vec3& direction) const = 0;
    virtual Vec3 generate() const = 0;
};

class SpherePDF : public Vec3PDF {
public:
    SpherePDF() = default;
    double value(const Vec3& direction) const override { return 1 / (4 * pi); }
    Vec3 generate() const override { return Vec3::random_unit(); }
};

class CosinePDF : public Vec3PDF {
    OrthonormalBasis uvw;

public:
    CosinePDF(const Vec3& w) : uvw(w) {}

    double value(const Vec3& direction) const override {
        auto cosine = dot(direction.normalized(), uvw.w());
        return (cosine <= 0) ? 0.0 : (cosine / pi);
    }

    Vec3 generate() const override { return uvw.transform(Vec3::random_cosine_z()); }
};

class EmissionPDF : public Vec3PDF {
    const Emitable& objects;
    Point3 origin;

public:
    EmissionPDF(const Emitable& objects, const Point3& origin) : objects(objects), origin(origin) {}
    double value(const Vec3& direction) const override {
        return objects.pdf_value(origin, direction);
    }
    Vec3 generate() const override { return objects.random(origin); }
};

class MixturePDF : public Vec3PDF {
    const Vec3PDF &p0, &p1;
    double weight_1;

public:
    MixturePDF(const Vec3PDF& p0, const Vec3PDF& p1, double w1 = 0.5)
        : p0(p0), p1(p1), weight_1(w1) {}
    double value(const Vec3& direction) const override {
        return (1 - weight_1) * p0.value(direction) + weight_1 * p1.value(direction);
    }

    Vec3 generate() const override {
        if (random_double() < weight_1) {
            return p1.generate();
        } else {
            return p0.generate();
        }
    }
};
