#include "camera.h"
#include "export.h"
#include "material.h"
#include "shape.h"
#include "sphere.h"
#include "texture.h"

#include <fstream>

namespace demo::cornell {

inline auto construct_camera() {
    Camera cam;
    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 800;
    cam.samples_per_pixel = 100;
    cam.max_depth = 50;
    cam.vfov = 40.0;
    cam.lookfrom = Point3(278, 278, -800);
    cam.lookat = Point3(278, 278, 0);
    cam.vup = Vec3(0, 1, 0);
    cam.defocus_angle = 0.0;
    cam.background = Color::black();
    return cam;
}

inline int main(int argc, char** argv) {
    if (argc < 2) return 1;

    auto camera = construct_camera();

    Objects world;

    auto red = std::make_shared<Lambertian>(Color(0.65, 0.05, 0.05));
    auto white = std::make_shared<Lambertian>(Color(0.73, 0.73, 0.73));
    auto green = std::make_shared<Lambertian>(Color(0.12, 0.45, 0.15));
    auto light = std::make_shared<Light>(Color::white() * 15.0);

    world.add(
        std::make_shared<Quadrilateral>(
            Point3(555, 0, 0), Vec3(0, 555, 0), Vec3(0, 0, 555), green));  // left
    world.add(
        std::make_shared<Quadrilateral>(
            Point3(0, 0, 0), Vec3(0, 555, 0), Vec3(0, 0, 555), red));  // right
    world.add(
        std::make_shared<Quadrilateral>(
            Point3(343, 554, 332), Vec3(-130, 0, 0), Vec3(0, 0, -105), light));  // light
    world.add(
        std::make_shared<Quadrilateral>(
            Point3(0, 0, 0), Vec3(555, 0, 0), Vec3(0, 0, 555), white));  // floor
    world.add(
        std::make_shared<Quadrilateral>(
            Point3(0, 0, 555), Vec3(555, 0, 0), Vec3(0, 555, 0), white));  // back
    world.add(
        std::make_shared<Quadrilateral>(
            Point3(555, 555, 555), Vec3(-555, 0, 0), Vec3(0, 0, -555), red));  // ceiling
    auto image = camera.render(world, true);

    auto file = std::ofstream(argv[1], std::ios::binary | std::ios::out);
    dump(image, file);
    return 0;
}

}  // namespace demo::cornell
