#include "render/render.hpp"

#include <format>
#include <fstream>
#include <iostream>
#include <spdlog/spdlog.h>
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

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << std::format("Usage: {} <output_image_path>\n", argv[0]);
        return 1;
    }
    const char* output_image_path = argv[1];

    using CoordT = float;
    using RenderT = float;

    using S = Sphere<CoordT, RenderT>;
    using Camera = Camera<CoordT>;

    std::vector<S> spheres;
    spheres.push_back(
        S::conf()
            .center(0.0, -10004, -20)
            .radius(10000)
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

    auto camera = Camera{
        .width = 1280,
        .height = 720,
    };
    auto image = render(camera, spheres, Vec3<RenderT>{2.}, 5);
    std::ofstream ofs(output_image_path, std::ios::binary | std::ios::out);
    dump(image, camera.width, camera.height, ofs);
    ofs.close();
    return 0;
}
