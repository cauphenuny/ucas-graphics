#pragma once

#include "aabb.h"
#include "hittable.h"
#include "utility.h"

#include <algorithm>
#include <memory>

class BVHNode : public Hittable {
    std::shared_ptr<Hittable> left, right;
    BoundingBox bbox;

public:
    BVHNode(const Objects& objects) : BVHNode(objects.items(), 0, objects.items().size()) {}

    BVHNode(std::vector<std::shared_ptr<Hittable>> objects, size_t start, size_t end) {
        bbox = BoundingBox::empty();
        for (size_t i = start; i < end; i++) {
            bbox = BoundingBox::combine(bbox, objects[i]->bounding_box());
        }
        int axis = bbox.longest_axis();

        size_t object_span = end - start;
        if (object_span == 1) {
            left = right = objects[start];
        } else if (object_span == 2) {
            left = objects[start];
            right = objects[start + 1];
        } else {
            std::sort(objects.begin() + start, objects.begin() + end, comparator[axis]);
            auto mid = start + object_span / 2;
            left = std::make_shared<BVHNode>(objects, start, mid);
            right = std::make_shared<BVHNode>(objects, mid, end);
        }
    }

    static bool
    box_compare(const std::shared_ptr<Hittable>& a, const std::shared_ptr<Hittable>& b, int axis) {
        BoundingBox box_a = a->bounding_box();
        BoundingBox box_b = b->bounding_box();
        return box_a.axis_interval(axis).min < box_b.axis_interval(axis).min;
    }

    static bool
    box_x_compare(const std::shared_ptr<Hittable>& a, const std::shared_ptr<Hittable>& b) {
        return box_compare(a, b, 0);
    }
    static bool
    box_y_compare(const std::shared_ptr<Hittable>& a, const std::shared_ptr<Hittable>& b) {
        return box_compare(a, b, 1);
    }
    static bool
    box_z_compare(const std::shared_ptr<Hittable>& a, const std::shared_ptr<Hittable>& b) {
        return box_compare(a, b, 2);
    }

    constexpr static bool (*const comparator[3])(
        const std::shared_ptr<Hittable>& a,
        const std::shared_ptr<Hittable>& b) = {box_x_compare, box_y_compare, box_z_compare};

    bool hit(const Ray& ray, Interval interval, HitResult& result) const override {
        if (!bbox.hit(ray, interval)) {
            return false;
        }
        bool hit_left = left->hit(ray, interval, result);
        bool hit_right =
            right->hit(ray, hit_left ? Interval(interval.min, result.t) : interval, result);

        return hit_left || hit_right;
    }

    BoundingBox bounding_box() const override { return bbox; }
};
