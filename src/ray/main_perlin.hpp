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

    HittableList world;

    auto marble_texture = MarbleTexture::create(3., Vec3(-0.6, 0, 1));
    auto turb_texture = TurbulenceTexture::create(2.);
    world.add(Sphere::create(Point3(0, -1000, 0), 1000, Lambertian::create(marble_texture)));
    world.add(Sphere::create(Point3(0, 2, 0), 2.0, Lambertian::create(turb_texture)));

    auto image = camera.render(world, true);

    auto file = std::ofstream(argv[1], std::ios::binary | std::ios::out);
    dump(image, file);
    return 0;
}

}  // namespace demo::perlin
