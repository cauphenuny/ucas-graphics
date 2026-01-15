#pragma once

#include "interval.h"
#include "ray.h"
#include "vec.h"

struct HitResult {
    Point3 p;
    Vec3 normal;
    double t;
    bool front_face;

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
};

class Objects : public Hittable {
    std::vector<std::shared_ptr<Hittable>> objects;

public:
    Objects() = default;
    Objects(const std::shared_ptr<Hittable>& object) { add(object); }

    void clear() { objects.clear(); }

    void add(const std::shared_ptr<Hittable>& object) { objects.push_back(object); }

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
