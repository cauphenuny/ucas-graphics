#pragma once
#include "hittable.h"

#include <memory>
#include <tuple>
#include <utility>

class Translation : public Hittable, public traits::CreateShared<Translation> {
    std::shared_ptr<Hittable> object;
    Vec3 offset;
    BoundingBox bbox;

public:
    Translation(std::shared_ptr<Hittable> obj, double x, double y, double z)
        : Translation(std::move(obj), Vec3(x, y, z)) {}
    Translation(std::shared_ptr<Hittable> obj, const Vec3& displacement)
        : object(std::move(obj)), offset(displacement) {
        bbox = object->bounding_box() + offset;
    }
    BoundingBox bounding_box() const override { return bbox; }
    bool hit(const Ray& ray, Interval interval, HitResult& result) const override {
        Ray moved_ray = ray.redirect(ray.origin() - offset, ray.direction());
        if (!object->hit(moved_ray, interval, result)) return false;
        result.p += offset;
        return true;
    }
};

namespace transform {

template <int axis>
    requires(axis >= 0) && (axis < 3)
Vec3 rotate(const Vec3 raw, double cos, double sin) {
    constexpr int cos_axis = (axis + 1) % 3;
    constexpr int sin_axis = (axis + 2) % 3;
    Vec3 rotated;
    rotated[axis] = raw[axis];
    rotated[cos_axis] = cos * raw[cos_axis] - sin * raw[sin_axis];
    rotated[sin_axis] = sin * raw[cos_axis] + cos * raw[sin_axis];
    return rotated;
}

}  // namespace transform

template <int axis>
    requires(axis >= 0) && (axis < 3)
class Rotation : public Hittable, public traits::CreateShared<Rotation<axis>> {
    std::shared_ptr<Hittable> object;
    double sin_theta, cos_theta;
    BoundingBox bbox;

public:
    Rotation(std::shared_ptr<Hittable> obj, double angle) : object(std::move(obj)) {
        auto rad = degrees_to_radians(angle);
        sin_theta = std::sin(rad);
        cos_theta = std::cos(rad);
        bbox = object->bounding_box();

        Point3 min(infinity, infinity, infinity);
        Point3 max(-infinity, -infinity, -infinity);

        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 2; j++) {
                for (int k = 0; k < 2; k++) {
                    auto x = i * bbox.x.max + (1 - i) * bbox.x.min;
                    auto y = j * bbox.y.max + (1 - j) * bbox.y.min;
                    auto z = k * bbox.z.max + (1 - k) * bbox.z.min;
                    Point3 rotated = transform::rotate<axis>(Point3(x, y, z), cos_theta, sin_theta);
                    for (int c = 0; c < 3; c++) {
                        min[c] = std::min(min[c], rotated[c]);
                        max[c] = std::max(max[c], rotated[c]);
                    }
                }
            }
        }
    }
    BoundingBox bounding_box() const override { return bbox; }
    bool hit(const Ray& ray, Interval interval, HitResult& result) const override {
        // from world space to object space
        auto origin = transform::rotate<axis>(ray.origin(), cos_theta, -sin_theta);
        auto direction = transform::rotate<axis>(ray.direction(), cos_theta, -sin_theta);

        Ray rotated_ray = ray.redirect(origin, direction);

        if (!object->hit(rotated_ray, interval, result)) return false;

        // from object space to world space
        result.p = transform::rotate<axis>(result.p, cos_theta, sin_theta);
        result.normal = transform::rotate<axis>(result.normal, cos_theta, sin_theta);

        return true;
    }
};

using RotationX = Rotation<0>;
using RotationY = Rotation<1>;
using RotationZ = Rotation<2>;

template <typename Operation, typename... Args> class TransformOperator {
    std::tuple<Args...> args;

public:
    explicit TransformOperator(Args... args) : args(std::make_tuple(std::move(args)...)) {}

    template <IsHittable T>
    friend std::shared_ptr<Operation> operator|(std::shared_ptr<T> obj, TransformOperator op) {
        return std::apply(
            [&](auto&&... unpacked) {
                return std::make_shared<Operation>(
                    std::move(obj), std::forward<decltype(unpacked)>(unpacked)...);
            },
            std::move(op.args));
    }
};

template <typename... Args> using Translate = TransformOperator<Translation, Args...>;

template <int axis>
    requires(axis >= 0) && (axis < 3)
using Rotate = TransformOperator<Rotation<axis>, double>;

using RotateX = Rotate<0>;
using RotateY = Rotate<1>;
using RotateZ = Rotate<2>;
