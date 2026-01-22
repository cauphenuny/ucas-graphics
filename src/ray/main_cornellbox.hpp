#include "camera.h"
#include "export.h"
#include "material.h"
#include "medium.h"
#include "shape.h"
#include "sphere.h"
#include "transform.h"

#include <fstream>

namespace demo::cornell {

inline auto construct_camera() {
    Camera cam;
    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 800;
    cam.samples_per_pixel = 10000;
    cam.max_depth = 50;
    cam.vfov = 40.0;
    cam.lookfrom = Point3(278, 278, -800);
    cam.lookat = Point3(278, 278, 0);
    cam.vup = Vec3(0, 1, 0);
    cam.defocus_angle = 0.0;
    cam.background = Color::black();
    return cam;
}

inline auto
construct_wall(HittableList& world, auto&& green, auto&& red, auto&& white, auto&& emit) {
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
    bool metalize_box1 = false;
    bool glassize_box2 = false;
    if (argc > 2) {
        for (int i = 2; i < argc; i++) {
            if (std::string_view(argv[i]) == "--smoke") {
                smoke = true;
            }
            if (std::string_view(argv[i]) == "--light") {
                sample_light = true;
            }
            if (std::string_view(argv[i]) == "--metal") {
                metalize_box1 = true;
            }
            if (std::string_view(argv[i]) == "--glass") {
                glassize_box2 = true;
            }
        }
    }
    std::clog << std::format(
        "Arguments: smoke={}, sample_light={}, metalize_box1={}, glassize_box2={}\n", smoke,
        sample_light, metalize_box1, glassize_box2);

    auto camera = construct_camera();

    HittableList world;
    SamplableList samples;

    auto red = Lambertian::create(Color(0.65, 0.05, 0.05));
    auto white = Lambertian::create(Color(0.73, 0.73, 0.73));
    auto green = Lambertian::create(Color(0.12, 0.45, 0.15));
    auto metal = Metal::create(Color(0.8, 0.85, 0.88), 0.0);
    auto emit = Light::create(Color::white() * 15.0);
    auto glass = Dielectric::create(1.2, 0.2);

    construct_wall(world, green, red, white, emit);

    auto light =
        Quadrilateral::create(Point3(343, 554, 332), Vec3(-130, 0, 0), Vec3(0, 0, -105), emit);

    std::shared_ptr<Material> box1_material;
    if (metalize_box1) {
        box1_material = metal;
    } else {
        box1_material = white;
    }
    std::shared_ptr<Hittable> box1 =
        Box::create(Point3(0, 0, 0), Point3(165, 330, 165), box1_material);

    box1 = box1 | RotateY(15) | Translate(Vec3(265, 0, 295));

    auto sphere2 = Sphere::create(Point3(200, 140, 200), 140, glass);
    auto box2 = Box::create(Point3(0, 0, 0), Point3(165, 165, 165), white) | RotateY(-18) |
                Translate(Vec3(130, 0, 65));

    std::shared_ptr<Hittable> object2;
    if (glassize_box2) {
        object2 = sphere2;
    } else {
        object2 = box2;
    }

    if (smoke) {
        world.add(ConstantMedium::create(box1, 0.01, Color::black()));
        world.add(ConstantMedium::create(object2, 0.01, Color::white()));
    } else {
        world.add(box1);
        world.add(object2);
    }

    if (sample_light) {
        samples.add(light);
    }

    auto image = camera.render(world, true, samples.size() ? &samples : nullptr);

    auto file = std::ofstream(argv[1], std::ios::binary | std::ios::out);
    dump(image, file);
    return 0;
}

}  // namespace demo::cornell
