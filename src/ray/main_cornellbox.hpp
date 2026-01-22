#include "camera.h"
#include "export.h"
#include "material.h"
#include "medium.h"
#include "shape.h"
#include "transform.h"

#include <fstream>

namespace demo::cornell {

inline auto construct_camera() {
    Camera cam;
    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 800;
    cam.samples_per_pixel = 1000;
    cam.max_depth = 50;
    cam.vfov = 40.0;
    cam.lookfrom = Point3(278, 278, -800);
    cam.lookat = Point3(278, 278, 0);
    cam.vup = Vec3(0, 1, 0);
    cam.defocus_angle = 0.0;
    cam.background = Color::black();
    return cam;
}

inline auto construct_wall(Objects& world, auto&& green, auto&& red, auto&& white, auto&& emit) {
    world.add(
        std::make_shared<Quadrilateral>(
            Point3(555, 0, 0), Vec3(0, 555, 0), Vec3(0, 0, 555), green));  // left
    world.add(
        std::make_shared<Quadrilateral>(
            Point3(0, 0, 0), Vec3(0, 555, 0), Vec3(0, 0, 555), red));  // right
    world.add(
        std::make_shared<Quadrilateral>(
            Point3(343, 554, 332), Vec3(-130, 0, 0), Vec3(0, 0, -105), emit));  // light
    world.add(
        std::make_shared<Quadrilateral>(
            Point3(0, 0, 0), Vec3(555, 0, 0), Vec3(0, 0, 555), white));  // floor
    world.add(
        std::make_shared<Quadrilateral>(
            Point3(0, 0, 555), Vec3(555, 0, 0), Vec3(0, 555, 0), white));  // back
    world.add(
        std::make_shared<Quadrilateral>(
            Point3(555, 555, 555), Vec3(-555, 0, 0), Vec3(0, 0, -555), white));  // ceiling
}

inline int main(int argc, char** argv) {
    if (argc < 2) return 1;
    bool smoke = false;
    bool sample_light = false;
    if (argc > 2) {
        for (int i = 2; i < argc; i++) {
            if (std::string_view(argv[i]) == "--smoke") {
                smoke = true;
            }
            if (std::string_view(argv[i]) == "--sample_light") {
                sample_light = true;
            }
        }
    }

    auto camera = construct_camera();

    Objects world;

    auto red = Lambertian::create(Color(0.65, 0.05, 0.05));
    auto white = Lambertian::create(Color(0.73, 0.73, 0.73));
    auto green = Lambertian::create(Color(0.12, 0.45, 0.15));
    auto emit = Light::create(Color::white() * 15.0);

    construct_wall(world, green, red, white, emit);

    Quadrilateral light(Point3(343, 554, 332), Vec3(-130, 0, 0), Vec3(0, 0, -105), emit);

    std::shared_ptr<Hittable> box1 = Box::create(Point3(0, 0, 0), Point3(165, 330, 165), white);

    box1 = box1 | RotateY(15) | Translate(Vec3(265, 0, 295));

    std::shared_ptr<Hittable> box2 = Box::create(Point3(0, 0, 0), Point3(165, 165, 165), white);

    box2 = box2 | RotateY(-18) | Translate(Vec3(130, 0, 65));

    if (smoke) {
        world.add(ConstantMedium::create(box1, 0.01, Color::black()));
        world.add(ConstantMedium::create(box2, 0.01, Color::white()));
    } else {
        world.add(box1);
        world.add(box2);
    }

    if (sample_light) {
        camera.samples_per_pixel = 10;
    }

    auto image = camera.render(world, true, sample_light ? &light : nullptr);

    auto file = std::ofstream(argv[1], std::ios::binary | std::ios::out);
    dump(image, file);
    return 0;
}

}  // namespace demo::cornell
