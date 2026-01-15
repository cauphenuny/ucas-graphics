#include "render/render.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <fstream>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <vector>

template <typename T>
void dump(std::vector<Vec3<T>> image, int width, int height, std::ofstream& ofs) {
    ofs << std::format("P6\n{} {}\n255\n", width, height);
    for (unsigned i = 0; i < width * height; ++i) {
        unsigned char r = (unsigned char)(std::min(T(1), image[i].x) * 255);
        unsigned char g = (unsigned char)(std::min(T(1), image[i].y) * 255);
        unsigned char b = (unsigned char)(std::min(T(1), image[i].z) * 255);
        ofs << r << g << b;
    }
}

using CoordT = double;
using RenderT = double;

using S = Sphere<CoordT, RenderT>;
using Cam = Camera<CoordT>;

auto build() {
    std::vector<S> spheres;
    spheres.push_back(
        S::conf()
            .center(0.0, -1000004, -20)
            .radius(1000000)
            .surface_color(0.2, 0.2, 0.2)
            .emission_color(0));
    spheres.push_back(
        S::conf()
            .center(0.0, 0, -20)
            .radius(4)
            .surface_color(1.0, 0.32, 0.36)
            .reflection(1)
            .transparency(0.5));
    spheres.push_back(
        S::conf()
            .center(5.0, -1, -15)
            .radius(2)
            .surface_color(0.90, 0.76, 0.46)
            .reflection(1)
            .transparency(0));
    spheres.push_back(
        S::conf()
            .center(5.0, 0, -25)
            .radius(3)
            .surface_color(0.4, 0.55, 0.8)
            .reflection(1)
            .transparency(0));
    spheres.push_back(
        S::conf()
            .center(-5.5, 0, -15)
            .radius(3)
            .surface_color(0.9, 0.9, 0.9)
            .reflection(1)
            .transparency(0));

    // light
    spheres.push_back(S::conf().center(0.0, 20, -30).radius(3).emission_color(1));

    return spheres;
}

int main(int argc, char** argv) {
    auto scene = build();

    auto camera = Cam{
        .width = 1280,
        .height = 800,
        .position = {0, 0, 10},
        .look_at = {0, 0, -20},
    };

    Vec3<CoordT> offset = camera.position - camera.look_at;
    CoordT radius = offset.norm();
    double yaw = std::atan2(offset.x, offset.z);
    double pitch = std::asin(offset.y / radius);
    const double pi = std::acos(-1.0);
    const double pitch_limit = pi / 2.0 - 0.01;
    constexpr double angle_step = 0.02, radius_step = 0.5;

    auto update_position = [&]() {
        CoordT cos_pitch = std::cos(pitch);
        camera.position.x = camera.look_at.x + radius * cos_pitch * std::sin(yaw);
        camera.position.z = camera.look_at.z + radius * cos_pitch * std::cos(yaw);
        camera.position.y = camera.look_at.y + radius * std::sin(pitch);
    };

    cv::namedWindow("path_tracer", cv::WINDOW_AUTOSIZE);

    std::vector<Vec3<RenderT>> image;
    auto render_frame = [&]() {
        image = render(camera, scene, Vec3<RenderT>{2}, 5);
        cv::Mat frame(camera.height, camera.width, CV_8UC3);
        for (unsigned y = 0; y < camera.height; ++y) {
            for (unsigned x = 0; x < camera.width; ++x) {
                auto pixel = image[y * camera.width + x];
                auto to_byte = [](RenderT value) {
                    return static_cast<unsigned char>(
                        std::clamp(value, RenderT(0), RenderT(1)) * 255);
                };
                frame.at<cv::Vec3b>(y, x) =
                    cv::Vec3b(to_byte(pixel.z), to_byte(pixel.y), to_byte(pixel.x));
            }
        }
        cv::imshow("path_tracer", frame);
    };

    render_frame();

    std::cerr << "Running path tracer. Use 'h','j','k','l' to rotate camera, 'i' and 'o' to zoom "
                 "in and out. Press ESC to exit."
              << std::endl;

    bool running = true;
    while (running) {
        int key = cv::waitKey(30);
        bool moved = false;
        switch (key) {
            case 'h':
                yaw -= angle_step;
                moved = true;
                break;
            case 'l':
                yaw += angle_step;
                moved = true;
                break;
            case 'j':
                pitch = std::max(-pitch_limit, pitch - angle_step);
                moved = true;
                break;
            case 'k':
                pitch = std::min(pitch_limit, pitch + angle_step);
                moved = true;
                break;
            case 'i':
                radius = std::max(1.0, radius - radius_step);
                moved = true;
                break;
            case 'o':
                radius += radius_step;
                moved = true;
                break;
            case 27: running = false; break;
            default: break;
        }
        if (moved) {
            update_position();
            render_frame();
        }
    }

    cv::destroyWindow("path_tracer");
    return 0;
}
