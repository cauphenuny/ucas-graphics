#pragma once

#include "hittable.h"
#include "material.h"
#include "pdf.h"

#include <atomic>
#include <format>
#include <iostream>
#include <vector>

struct RenderResult {
    int width, height;
    std::vector<Color> data;
};

struct Camera {
    double aspect_ratio = 1.0;
    int image_width = 100;
    int samples_per_pixel = 10;
    int max_depth = 10;  // max depth of ray bounces
    Color background = Color(0.7, 0.8, 1);

    double vfov = 90.0;  // vetical fov, unit: angle
    Point3 lookfrom = Point3(0, 0, 0);
    Point3 lookat = Point3(0, 0, -1);
    Vec3 vup = Vec3(0, 1, 0);

    double defocus_angle = 0;  // variation angle of rays through each pixel, unit: angle
    double focus_dist = 10;

    auto render(const Hittable& world, bool verbose = false, Emitable* lights = nullptr)
        -> RenderResult {
        initialize();
        auto image = std::vector<Color>(image_width * image_height);

        std::atomic<int> counter = 0;
#pragma omp parallel for
        for (int j = 0; j < image_height; ++j) {
            for (int i = 0; i < image_width; ++i) {
                Color pixel_color(0, 0, 0);
                for (int sj = 0; sj < sqrt_spp; sj++) {
                    for (int si = 0; si < sqrt_spp; si++) {
                        Ray r = get_ray(i, j, si, sj);
                        pixel_color += ray_color(r, world, lights, max_depth);
                    }
                }
                image[j * image_width + i] = pixel_color * pixel_samples_scale;
            }
            counter++;
            if (verbose) {
                std::clog << std::format("Rendered {}/{}\n", counter.load(), image_height)
                          << std::flush;
            }
        }
        return RenderResult{.width = image_width, .height = image_height, .data = std::move(image)};
    }

private:
    int image_height;
    double pixel_samples_scale;
    int sqrt_spp;
    double recip_sqrt_spp;
    Point3 center;       // camera center
    Point3 pixel00_loc;  // Location of pixel (0,0)
    Vec3 pixel_delta_u;  // offset of one pixel in u-direction
    Vec3 pixel_delta_v;  // offset of one pixel in v-direction
    Vec3 u, v, w;
    Vec3 defocus_disk_u;
    Vec3 defocus_disk_v;

    void initialize() {
        image_height = std::max(static_cast<int>(image_width / aspect_ratio), 1);
        auto ratio = (double)image_width / (double)image_height;
        center = lookfrom;

        sqrt_spp = int(std::sqrt(samples_per_pixel));
        pixel_samples_scale = 1.0 / (sqrt_spp * sqrt_spp);
        recip_sqrt_spp = 1.0 / sqrt_spp;

        // viewport dimensions
        auto theta = degrees_to_radians(vfov);
        auto h = std::tan(theta / 2);
        auto viewport_height = 2.0 * focus_dist * h;
        auto viewport_width = ratio * viewport_height;

        w = (lookfrom - lookat).normalized();
        u = cross(vup, w).normalized();
        v = cross(w, u);

        // calculate vectors and delta-vectors
        auto viewport_u = viewport_width * u;
        auto viewport_v = viewport_height * -v;
        pixel_delta_u = viewport_u / image_width;
        pixel_delta_v = viewport_v / image_height;

        auto viewport_upper_left = center - focus_dist * w - viewport_u / 2 - viewport_v / 2;
        pixel00_loc = viewport_upper_left + pixel_delta_u / 2 + pixel_delta_v / 2;

        auto defocus_radius = focus_dist * std::tan(degrees_to_radians(defocus_angle) / 2);
        defocus_disk_u = defocus_radius * u;
        defocus_disk_v = defocus_radius * v;
    }

    auto sample_square() const { return Vec3{random_double() - 0.5, random_double() - 0.5, 0.0}; }

    auto sample_square_stratified(int si, int sj) const {
        auto px = ((si + random_double()) * recip_sqrt_spp) - 0.5;
        auto py = ((sj + random_double()) * recip_sqrt_spp) - 0.5;
        return Vec3{px, py, 0.0};
    }

    auto sample_defocus_disk() const {
        auto p = Vec3::random_in_unit_disk();
        return center + (p.x() * defocus_disk_u) + (p.y() * defocus_disk_v);
    }

    auto get_ray(int i, int j, int si, int sj) const -> Ray {
        auto offset = sample_square_stratified(si, sj);
        auto pixel_sample =
            pixel00_loc + ((i + offset.x()) * pixel_delta_u) + ((j + offset.y()) * pixel_delta_v);
        auto ray_origin = (defocus_angle <= 0) ? center : sample_defocus_disk();
        auto ray_time = random_double();
        return Ray(ray_origin, pixel_sample - ray_origin, ray_time);
    }

    auto ray_color(const Ray& ray, const Hittable& world, Emitable* lights, int depth)
        -> Vec3 const {
        if (depth <= 0) return Color(0, 0, 0);
        auto center = Point3(0, 0, -1);
        HitResult hit_result;
        if (!world.hit(ray, Interval(0.001, infinity), hit_result)) {
            return background;
        }
        ScatterResult scatter_result;
        Color color_emit = hit_result.mat->emit(ray, hit_result);
        double sampling_prob, scattering_prob;

        if (!hit_result.mat->scatter(ray, hit_result, scatter_result)) {
            return color_emit;
        }

        auto& [attenuation, scattered] = scatter_result;

        auto color_scatter = Match{std::move(scattered)}(
            [&](Ray ray) -> Color {
                return attenuation * ray_color(ray, world, lights, depth - 1);
            },
            [&](std::unique_ptr<Vec3PDF> pdf) -> Color {
                std::unique_ptr<Vec3PDF> scatter_pdf =
                    lights ? std::make_unique<MixturePDF>(EmissionPDF(*lights, hit_result.p), *pdf)
                           : std::move(pdf);
                auto scattered_ray = Ray(hit_result.p, scatter_pdf->generate(), ray.time());
                double sampling_prob = scatter_pdf->value(scattered_ray.direction());
                double scatter_prob =
                    hit_result.mat->scattering_pdf(ray, hit_result, scattered_ray);
                Color sampled_color = ray_color(scattered_ray, world, lights, depth - 1);
                return (attenuation * scatter_prob * sampled_color) / sampling_prob;
            });

        return color_emit + color_scatter;
    }
};
