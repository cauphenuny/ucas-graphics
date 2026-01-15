#pragma once

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

class Object {
public:
    virtual ~Object() = default;
    virtual bool hit(const Ray& r, double tmin, double tmax, HitResult& result) const = 0;
};

class ObjectSet : public Object {
    std::vector<std::shared_ptr<Object>> objects;

public:
    ObjectSet() = default;
    ObjectSet(const std::shared_ptr<Object>& object) { add(object); }

    void clear() { objects.clear(); }

    void add(const std::shared_ptr<Object>& object) { objects.push_back(object); }

    bool hit(const Ray& r, double tmin, double tmax, HitResult& result) const override {
        HitResult temp_result;
        bool hit_anything = false;
        double closest_so_far = tmax;

        for (const auto& object : objects) {
            if (object->hit(r, tmin, closest_so_far, temp_result)) {
                hit_anything = true;
                closest_so_far = temp_result.t;
                result = temp_result;
            }
        }
        return hit_anything;
    }
};
