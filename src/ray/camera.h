#pragma once

#include "hittable.h"
#include "material.h"
#include "pdf.h"
#include "spectrum.h"

#include <atomic>
#include <cmath>
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

    auto render(const Hittable& world, bool verbose = false, Samplable* sample_target = nullptr)
        -> RenderResult {
        initialize();
        auto image = std::vector<Color>(image_width * image_height);

        std::atomic<int> counter = 0;
#pragma omp parallel for
        for (int j = 0; j < image_height; ++j) {
            for (int i = 0; i < image_width; ++i) {
                Spectrum pixel_spectrum(0);
                int num_samples = 0;
                for (int sj = 0; sj < sqrt_spp; sj++) {
                    for (int si = 0; si < sqrt_spp; si++) {
                        Ray r = get_ray(i, j, si, sj);
                        auto intensity = ray_color(r, world, sample_target, max_depth);
                        if (!std::isfinite(intensity)) {
                            continue;
                        }
                        num_samples++;
                        // 累积到光谱的对应波长bin中
                        accumulate_spectral_sample(pixel_spectrum, intensity, r.wavelength());
                    }
                }
                // 归一化后转换为RGB
                if (num_samples > 0) {
                    pixel_spectrum *= (1.0 / num_samples);
                }
                image[j * image_width + i] = pixel_spectrum.toColor();
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
    int sqrt_spp;
    double recip_sqrt_spp;
    Point3 center;       // camera center
    Point3 pixel00_loc;  // Location of pixel (0,0)
    Vec3 pixel_delta_u;  // offset of one pixel in u-direction
    Vec3 pixel_delta_v;  // offset of one pixel in v-direction
    Vec3 u, v, w;
    Vec3 defocus_disk_u;
    Vec3 defocus_disk_v;
    Spectrum background_spectrum;
    double wavelength_range;
    double wavelength_pdf;

    void initialize() {
        image_height = std::max(static_cast<int>(image_width / aspect_ratio), 1);
        auto ratio = (double)image_width / (double)image_height;
        center = lookfrom;

        background_spectrum = Spectrum(background);
        wavelength_range = Spectrum::visible_max - Spectrum::visible_min;
        wavelength_pdf = 1.0 / wavelength_range;

        sqrt_spp = int(std::sqrt(samples_per_pixel));
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
        auto wavelength = random_double(Spectrum::visible_min, Spectrum::visible_max);
        return Ray(ray_origin, pixel_sample - ray_origin, wavelength, ray_time);
    }

    auto ray_color(const Ray& ray, const Hittable& world, Samplable* sample_target, int depth)
        -> double {
        if (depth <= 0) return 0.0;

        HitResult hit_result;
        if (!world.hit(ray, Interval(0.001, infinity), hit_result)) {
            return background_spectrum.value(ray.wavelength());
        }

        ScatterResult scatter_result;
        double emitted = hit_result.mat->emit(ray, hit_result);

        if (!hit_result.mat->scatter(ray, hit_result, scatter_result)) {
            return emitted;
        }

        auto& [attenuation, scattered] = scatter_result;
        if (attenuation <= 0.0) {
            return emitted;
        }

        double scatter_contrib = Match{std::move(scattered)}(
            [&](Ray scattered_ray) -> double {
                return attenuation * ray_color(scattered_ray, world, sample_target, depth - 1);
            },
            [&](std::unique_ptr<Vec3PDF> pdf) -> double {
                std::unique_ptr<Vec3PDF> scatter_pdf =
                    sample_target ? std::make_unique<MixturePDF>(
                                        std::make_unique<ObjectPDF>(*sample_target, hit_result.p),
                                        std::move(pdf))
                                  : std::move(pdf);
                auto scattered_ray = ray.redirect(hit_result.p, scatter_pdf->generate());
                double sampling_prob = scatter_pdf->value(scattered_ray.direction());
                if (sampling_prob <= 0.0) {
                    return 0.0;
                }
                double scatter_prob =
                    hit_result.mat->scattering_pdf(ray, hit_result, scattered_ray);
                double sampled = ray_color(scattered_ray, world, sample_target, depth - 1);
                return (attenuation * scatter_prob * sampled) / sampling_prob;
            });

        return emitted + scatter_contrib;
    }

    // 将单波长采样累积到Spectrum对象中
    void accumulate_spectral_sample(Spectrum& spectrum, double intensity, double wavelength) const {
        if (intensity <= 0.0) return;
        // 找到对应的bin并累加
        // 由于均匀采样波长，每个采样对整个光谱的贡献是 intensity * wavelength_range / nSamples
        // 但我们要写入对应的bin，所以直接按bin累加，最后均值会自动处理
        double t = (wavelength - Spectrum::lambdaStart) / (Spectrum::lambdaEnd - Spectrum::lambdaStart);
        int bin = static_cast<int>(t * Spectrum::nSamples);
        bin = std::clamp(bin, 0, Spectrum::nSamples - 1);
        // 乘以 nSamples 是因为我们均匀采样波长，但只写入一个bin
        // 这样当采样足够多时，每个bin的平均值就是正确的光谱值
        spectrum[bin] += intensity * Spectrum::nSamples;
    }
};
