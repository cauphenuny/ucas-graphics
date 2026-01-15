#include "camera.h"
#include "export.h"
#include "hittable.h"
#include "material.h"
#include "sphere.h"
#include "vec.h"

#include <fstream>

int main(int argc, char** argv) {
    if (argc < 2) return 1;

    Objects world;

    auto material_ground = std::make_shared<Lambertian>(Color(0.8, 0.8, 0.0));
    auto material_center = std::make_shared<Lambertian>(Color(0.1, 0.2, 0.5));
    auto material_left = std::make_shared<Dielectric>(1.5);
    auto material_bubble = std::make_shared<Dielectric>(1.0 / 1.5);
    auto meterial_right = std::make_shared<Metal>(Color(0.8, 0.6, 0.2), 1.0);

    world.add(std::make_shared<Sphere>(Point3(0.0, -100.5, -1.0), 100.0, material_ground));
    world.add(std::make_shared<Sphere>(Point3(0.0, 0.0, -1.2), 0.5, material_center));
    world.add(std::make_shared<Sphere>(Point3(-1.0, 0.0, -1.0), 0.5, material_left));
    world.add(std::make_shared<Sphere>(Point3(-1.0, 0.0, -1.0), 0.4, material_bubble));
    world.add(std::make_shared<Sphere>(Point3(1.0, 0.0, -1.0), 0.5, meterial_right));

    Camera cam;

    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 800;
    cam.samples_per_pixel = 100;

    cam.vfov = 20;
    cam.lookfrom = Point3(-2, 2, 1);
    cam.lookat = Point3(0, 0, -1);
    cam.vup = Vec3(0, 1, 0);

    auto image = cam.render(world);

    auto file = std::ofstream(argv[1], std::ios::binary | std::ios::out);
    dump(image, file);
    return 0;
}
