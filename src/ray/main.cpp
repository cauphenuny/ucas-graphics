#include "camera.h"
#include "export.h"
#include "hittable.h"
#include "ray.h"
#include "sphere.h"
#include "vec.h"

#include <fstream>

int main(int argc, char** argv) {
    if (argc < 2) return 1;
    Objects world;
    world.add(std::make_shared<Sphere>(Point3(0, 0, -1), 0.5));
    world.add(std::make_shared<Sphere>(Point3(0, -100.5, -1), 100));

    Camera cam;
    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 800;
    auto image = cam.render(world);

    auto file = std::ofstream(argv[1], std::ios::binary | std::ios::out);
    dump(image, file);
    return 0;
}
