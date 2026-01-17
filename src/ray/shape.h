#pragma once

#include "hittable.h"

#include <memory>

// polygon: triangle or quadrilateral
class Shape2D : public Hittable {
protected:
    Point3 origin;
    double coef_d;
    Vec3 vec_u, vec_v, normal, vec_w;
    std::shared_ptr<Material> mat;

    BoundingBox bbox;

    virtual void set_bounding_box() = 0;
    virtual bool is_interior(double alpha, double beta, HitResult& result) const = 0;

public:
    Shape2D(const Point3& origin, const Vec3& u, const Vec3& v, std::shared_ptr<Material> mat)
        : origin(origin), vec_u(u), vec_v(v), mat(mat) {
        auto n = cross(u, v);
        normal = n.normalized();
        coef_d = dot(normal, origin);
        vec_w = n / dot(n, n);
    }

    BoundingBox bounding_box() const override { return bbox; }

    bool hit(const Ray& ray, Interval interval, HitResult& result) const override {
        auto denom = dot(normal, ray.direction());
        if (std::fabs(denom) < 1e-8) return false;
        auto t = (coef_d - dot(normal, ray.origin())) / denom;
        if (!interval.contains(t)) return false;

        auto intersection = ray.at(t);
        auto vec_hit = intersection - origin;
        auto alpha = dot(vec_w, cross(vec_hit, vec_v));
        auto beta = dot(vec_w, cross(vec_u, vec_hit));

        if (!is_interior(alpha, beta, result)) return false;

        result.t = t;
        result.p = intersection;
        result.mat = mat;
        result.set_face_normal(ray, normal);

        return true;
    }
};

class Triangle : public Shape2D {
protected:
    void set_bounding_box() override {
        bbox = BoundingBox::diag(origin, origin + vec_u);
        bbox = BoundingBox::combine(bbox, BoundingBox::diag(origin, origin + vec_v));
        bbox = BoundingBox::combine(bbox, BoundingBox::diag(origin + vec_u, origin + vec_v));
    }
    bool is_interior(double alpha, double beta, HitResult& result) const override {
        if (alpha + beta > 1) return false;

        auto unit = Interval(0, 1);
        if (!unit.contains(alpha)) return false;
        if (!unit.contains(beta)) return false;

        result.u = alpha;
        result.v = beta;

        return true;
    }

public:
    Triangle(const Point3& origin, const Vec3& u, const Vec3& v, std::shared_ptr<Material> mat)
        : Shape2D(origin, u, v, mat) {
        set_bounding_box();
    }
};

class Quadrilateral : public Shape2D {
protected:
    void set_bounding_box() override {
        auto bbox_diag1 = BoundingBox::diag(origin, origin + vec_u);
        auto bbox_diag2 = BoundingBox::diag(origin + vec_v, origin + vec_u + vec_v);
        bbox = BoundingBox::combine(bbox_diag1, bbox_diag2);
    }
    bool is_interior(double alpha, double beta, HitResult& result) const override {
        auto unit = Interval(0, 1);

        if (!unit.contains(alpha)) return false;
        if (!unit.contains(beta)) return false;

        result.u = alpha;
        result.v = beta;

        return true;
    }

public:
    Quadrilateral(const Point3& origin, const Vec3& u, const Vec3& v, std::shared_ptr<Material> mat)
        : Shape2D(origin, u, v, mat) {
        set_bounding_box();
    }
};

class Ellipse : public Shape2D {
protected:
    double a_squared, b_squared;

    void set_bounding_box() override {
        auto r_u = vec_u.normalized() * std::sqrt(a_squared);
        auto r_v = vec_v.normalized() * std::sqrt(b_squared);
        bbox = BoundingBox::diag(origin - r_u - r_v, origin + r_u + r_v);
    }
    bool is_interior(double alpha, double beta, HitResult& result) const override {
        auto val = (alpha * alpha) / a_squared + (beta * beta) / b_squared;
        if (val > 1.0) return false;

        result.u = alpha / std::sqrt(a_squared);
        result.v = beta / std::sqrt(b_squared);

        return true;
    }

public:
    Ellipse(const Point3& origin, const Vec3& u, const Vec3& v, std::shared_ptr<Material> mat)
        : Shape2D(origin, u, v, mat) {
        a_squared = dot(u, u);
        b_squared = dot(v, v);
        set_bounding_box();
    }
};
