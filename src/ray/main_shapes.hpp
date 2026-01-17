#include "camera.h"
#include "export.h"
#include "material.h"
#include "shape.h"

#include <fstream>
#include <memory>

namespace demo::shapes {

inline auto construct_camera() {
    Camera cam;
    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 800;
    cam.samples_per_pixel = 400;
    cam.max_depth = 50;
    cam.vfov = 80.0;
    cam.lookfrom = Point3(0, 1, 9);
    cam.lookat = Point3(0, 0, 0);
    cam.vup = Vec3(0, 1, 0);
    cam.defocus_angle = 0.0;
    return cam;
}

inline int main(int argc, char** argv) {
    if (argc < 2) return 1;

    auto camera = construct_camera();

    Objects world;

    auto red = std::make_shared<Lambertian>(Color::red());
    auto green = std::make_shared<Lambertian>(Color::green());
    auto blue = std::make_shared<Lambertian>(Color::blue());
    auto orange = std::make_shared<Lambertian>(Color::orange());
    auto teal = std::make_shared<Lambertian>(Color::teal());
    auto gray = std::make_shared<Lambertian>(Color::gray());

    auto axis_x = Vec3(4, 0, 0);
    auto axis_y = Vec3(0, 4, 0);
    auto axis_z = Vec3(0, 0, 4);

    world.add(std::make_shared<Quadrilateral>(Point3(-3, -2, 5), -axis_z, axis_y, red));
    world.add(std::make_shared<Quadrilateral>(Point3(-2, -2, 0), axis_x, axis_y, green));
    for (int i = -9; i <= 9; i++) {
        world.add(std::make_shared<Ellipse>(Point3(0, 0.2 * i, 3), axis_x / 4, axis_z / 4, gray));
    }
    world.add(std::make_shared<Quadrilateral>(Point3(3, -2, 1), axis_z, axis_y, blue));
    world.add(std::make_shared<Quadrilateral>(Point3(-2, 3, 1), axis_x, axis_z, orange));
    world.add(std::make_shared<Quadrilateral>(Point3(-2, -3, 5), axis_x, -axis_z, teal));
    world.add(std::make_shared<Triangle>(Point3(-2, -2, 5), axis_x * 0.8, -axis_z * 0.8, orange));

    auto image = camera.render(world, true);

    auto file = std::ofstream(argv[1], std::ios::binary | std::ios::out);
    dump(image, file);
    return 0;
}

}  // namespace demo::shapes
