#pragma once

#include "aabb.h"
#include "interval.h"
#include "ray.h"
#include "utility.h"
#include "vec.h"

#include <memory>

class Material;

struct HitResult {
    Point3 p;
    Vec3 normal;
    double t;  // ray t
    bool front_face;
    std::shared_ptr<Material> mat;
    double u, v;  // texture coordinates

    /**
     * @brief set normal vector, considering the ray direction
     *
     * @param r ray
     * @param outward_normal the original normal vec from geometry code  NOTE: need to be unit-vec
     */
    void set_face_normal(const Ray& r, const Vec3& outward_normal) {
        front_face = dot(r.direction(), outward_normal) < 0;
        normal = front_face ? outward_normal : -outward_normal;
    }
};

class Hittable {
public:
    virtual ~Hittable() = default;
    virtual bool hit(const Ray& ray, Interval interval, HitResult& result) const = 0;
    virtual BoundingBox bounding_box() const = 0;
};

class Emitable {
public:
    virtual ~Emitable() = default;
    virtual double pdf_value(const Point3& o, const Vec3& v) const = 0;
    virtual Vec3 random(const Point3& o) const = 0;
};

template <typename T>
concept IsHittable = std::derived_from<T, Hittable>;

class Objects : public Hittable, public traits::CreateShared<Objects> {
    std::vector<std::shared_ptr<Hittable>> objects;
    BoundingBox bbox;

public:
    Objects() = default;
    Objects(const std::shared_ptr<Hittable>& object) { add(object); }

    const auto& items() const { return objects; }

    void clear() { objects.clear(); }

    void add(const std::shared_ptr<Hittable>& object) {
        objects.push_back(object);
        bbox = BoundingBox::combine(bbox, object->bounding_box());
    }

    BoundingBox bounding_box() const override { return bbox; }

    bool hit(const Ray& ray, Interval interval, HitResult& result) const override {
        HitResult temp_result;
        bool hit_anything = false;
        double closest_so_far = interval.max;

        for (const auto& object : objects) {
            if (object->hit(ray, Interval(interval.min, closest_so_far), temp_result)) {
                hit_anything = true;
                closest_so_far = temp_result.t;
                result = temp_result;
            }
        }
        return hit_anything;
    }
};
