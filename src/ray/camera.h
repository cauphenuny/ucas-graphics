#pragma once

#include "hittable.h"
#include "material.h"

#include <vector>

struct RenderResult {
    int width, height;
    std::vector<Color> data;
};

struct Camera {
public:
    double aspect_ratio = 1.0;
    int image_width = 100;
    int samples_per_pixel = 10;
    int max_depth = 10;  // max depth of ray bounces

    double vfov = 90.0;  // vetical fov, unit: angle
    Point3 lookfrom = Point3(0, 0, 0);
    Point3 lookat = Point3(0, 0, -1);
    Vec3 vup = Vec3(0, 1, 0);

    auto render(const Hittable& world) -> RenderResult {
        initialize();
        auto image = std::vector<Color>(image_width * image_height);

        for (int j = 0; j < image_height; ++j) {
            for (int i = 0; i < image_width; ++i) {
                Color pixel_color(0, 0, 0);
                for (int sample = 0; sample < samples_per_pixel; sample++) {
                    Ray r = get_ray(i, j);
                    pixel_color += ray_color(r, world, max_depth);
                }
                image[j * image_width + i] = pixel_color * pixel_samples_scale;
            }
        }
        return RenderResult{.width = image_width, .height = image_height, .data = std::move(image)};
    }

private:
    int image_height;
    Point3 center;       // camera center
    Point3 pixel00_loc;  // Location of pixel (0,0)
    Vec3 pixel_delta_u;  // offset of one pixel in u-direction
    Vec3 pixel_delta_v;  // offset of one pixel in v-direction
    Vec3 u, v, w;
    double pixel_samples_scale;

    void initialize() {
        image_height = std::max(static_cast<int>(image_width / aspect_ratio), 1);
        auto ratio = (double)image_width / (double)image_height;
        center = lookfrom;

        pixel_samples_scale = 1.0 / samples_per_pixel;

        // viewport dimensions
        auto focal_length = (lookfrom - lookat).norm();
        auto theta = degrees_to_radians(vfov);
        auto h = std::tan(theta / 2);
        auto viewport_height = 2.0 * h * focal_length;
        auto viewport_width = ratio * viewport_height;

        w = (lookfrom - lookat).normalized();
        u = cross(vup, w).normalized();
        v = cross(w, u);

        // calculate vectors and delta-vectors
        auto viewport_u = viewport_width * u;
        auto viewport_v = viewport_height * -v;
        pixel_delta_u = viewport_u / image_width;
        pixel_delta_v = viewport_v / image_height;

        auto viewport_upper_left = center - focal_length * w - viewport_u / 2 - viewport_v / 2;
        pixel00_loc = viewport_upper_left + pixel_delta_u / 2 + pixel_delta_v / 2;
    }

    auto sample_square() const { return Vec3{random_double() - 0.5, random_double() - 0.5, 0.0}; }

    auto get_ray(int i, int j) const -> Ray {
        auto offset = sample_square();
        auto pixel_sample =
            pixel00_loc + ((i + offset.x()) * pixel_delta_u) + ((j + offset.y()) * pixel_delta_v);
        return Ray(center, pixel_sample - center);
    }

    auto ray_color(const Ray& ray, const Hittable& world, int depth) -> Vec3 const {
        if (depth <= 0) return Color(0, 0, 0);
        auto center = Point3(0, 0, -1);
        HitResult hit_result;
        if (world.hit(ray, Interval(0.001, infinity), hit_result)) {
            Ray scattered;
            Color attenuation;
            if (hit_result.mat->scatter(ray, hit_result, attenuation, scattered)) {
                return attenuation * ray_color(scattered, world, depth - 1);
            }
            return Color(0, 0, 0);
        }

        Vec3 unit_direction = ray.direction().normalized();
        auto a = 0.5 * (unit_direction.y() + 1.0);
        return (1.0 - a) * Color(1., 1., 1.) + a * Color(0.5, 0.7, 1.0);
    }
};
