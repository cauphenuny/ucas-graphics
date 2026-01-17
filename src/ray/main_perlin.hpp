#include "camera.h"
#include "export.h"
#include "material.h"
#include "sphere.h"
#include "texture.h"

#include <fstream>

namespace demo::perlin {

inline auto construct_camera() {
    Camera cam;
    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 800;
    cam.samples_per_pixel = 100;
    cam.max_depth = 50;
    cam.vfov = 20.0;
    cam.lookfrom = Point3(13, 2, 3);
    cam.lookat = Point3(0, 0, 0);
    cam.vup = Vec3(0, 1, 0);
    cam.defocus_angle = 0.0;
    return cam;
}

inline int main(int argc, char** argv) {
    if (argc < 2) return 1;

    auto camera = construct_camera();

    Objects world;

    auto marble_texture = std::make_shared<MarbleTexture>(3., Vec3(-0.6, 0, 1));
    auto turb_texture = std::make_shared<TurbulenceTexture>(2.);
    world.add(
        std::make_shared<Sphere>(
            Point3(0, -1000, 0), 1000, std::make_shared<Lambertian>(marble_texture)));
    world.add(
        std::make_shared<Sphere>(Point3(0, 2, 0), 2.0, std::make_shared<Lambertian>(turb_texture)));

    auto image = camera.render(world, true);

    auto file = std::ofstream(argv[1], std::ios::binary | std::ios::out);
    dump(image, file);
    return 0;
}

}  // namespace demo::perlin
