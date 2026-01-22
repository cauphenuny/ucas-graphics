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

    HittableList world;

    auto red = Lambertian::create(Color::red());
    auto green = Lambertian::create(Color::green());
    auto blue = Lambertian::create(Color::blue());
    auto orange = Lambertian::create(Color::orange());
    auto teal = Lambertian::create(Color::teal());
    auto gray = Lambertian::create(Color::gray());

    auto axis_x = Vec3(4, 0, 0);
    auto axis_y = Vec3(0, 4, 0);
    auto axis_z = Vec3(0, 0, 4);

    world.add(Quadrilateral::create(Point3(-3, -2, 5), -axis_z, axis_y, red));
    world.add(Quadrilateral::create(Point3(-2, -2, 0), axis_x, axis_y, green));
    for (int i = -9; i <= 9; i++) {
        world.add(Ellipse::create(Point3(0, 0.2 * i, 3), axis_x / 4, axis_z / 4, gray));
    }
    world.add(Quadrilateral::create(Point3(3, -2, 1), axis_z, axis_y, blue));
    world.add(Quadrilateral::create(Point3(-2, 3, 1), axis_x, axis_z, orange));
    world.add(Quadrilateral::create(Point3(-2, -3, 5), axis_x, -axis_z, teal));
    world.add(Triangle::create(Point3(-2, -2, 5), axis_x * 0.8, -axis_z * 0.8, orange));

    auto image = camera.render(world, true);

    auto file = std::ofstream(argv[1], std::ios::binary | std::ios::out);
    dump(image, file);
    return 0;
}

}  // namespace demo::shapes
