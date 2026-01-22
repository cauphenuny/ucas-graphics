#pragma once

#include "hittable.h"
#include "spectrum.h"

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

class Triangle : public Shape2D, public traits::CreateShared<Triangle> {
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

class Quadrilateral : public Shape2D, public Samplable, public traits::CreateShared<Quadrilateral> {
    double area;

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
        area = cross(u, v).norm();
        set_bounding_box();
    }

    double pdf_value(const Point3& o, const Vec3& v) const override {
        HitResult result;
        Ray ray(o, v, Spectrum::visible_min);
        if (!hit(ray, Interval(0.001, infinity), result)) return 0.0;

        auto distance_squared = result.t * result.t * v.sqrnorm();
        auto cosine = std::fabs(dot(normal, v.normalized()));

        return distance_squared / (cosine * area);
    }

    Vec3 random(const Point3& o) const override {
        auto random_point = origin + random_double() * vec_u + random_double() * vec_v;
        return random_point - o;
    }
};

class Ellipse : public Shape2D, public traits::CreateShared<Ellipse> {
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

class Box : public Hittable, public traits::CreateShared<Box> {
    HittableList sides;

public:
    bool hit(const Ray& ray, Interval interval, HitResult& result) const {
        return sides.hit(ray, interval, result);
    }
    BoundingBox bounding_box() const { return sides.bounding_box(); }

    Box(const Point3& a, const Point3& b, std::shared_ptr<Material> mat) {
        auto min =
            Point3(std::fmin(a.x(), b.x()), std::fmin(a.y(), b.y()), std::fmin(a.z(), b.z()));
        auto max =
            Point3(std::fmax(a.x(), b.x()), std::fmax(a.y(), b.y()), std::fmax(a.z(), b.z()));

        auto dx = Vec3(max.x() - min.x(), 0, 0);
        auto dy = Vec3(0, max.y() - min.y(), 0);
        auto dz = Vec3(0, 0, max.z() - min.z());

        sides.add(
            make_shared<Quadrilateral>(Point3(min.x(), min.y(), max.z()), dx, dy, mat));  // front
        sides.add(
            make_shared<Quadrilateral>(Point3(max.x(), min.y(), max.z()), -dz, dy, mat));  // right
        sides.add(
            make_shared<Quadrilateral>(Point3(max.x(), min.y(), min.z()), -dx, dy, mat));  // back
        sides.add(
            make_shared<Quadrilateral>(Point3(min.x(), min.y(), min.z()), dz, dy, mat));  // left
        sides.add(
            make_shared<Quadrilateral>(Point3(min.x(), max.y(), max.z()), dx, -dz, mat));  // top
        sides.add(
            make_shared<Quadrilateral>(Point3(min.x(), min.y(), min.z()), dx, dz, mat));  // bottom
    }
};
