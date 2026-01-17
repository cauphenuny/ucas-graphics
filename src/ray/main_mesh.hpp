#include "camera.h"
#include "export.h"
#include "material.h"
#include "mesh.h"
#include "sphere.h"
#include "texture.h"

#include <format>
#include <fstream>
#include <iostream>
#include <meshark/mesh-io.h>

namespace demo::mesh {

inline auto construct_camera() {
    Camera cam;
    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 1600;
    cam.samples_per_pixel = 100;
    cam.max_depth = 50;
    cam.vfov = 20.0;
    cam.lookfrom = Point3(-8, 20, 20);
    cam.lookat = Point3(0, 5, 0);
    cam.vup = Vec3(0, 1, 0);
    cam.defocus_angle = 0.0;
    return cam;
}

inline int main(int argc, char** argv) {
    if (argc < 3) return 1;
    auto input_obj = argv[1];
    auto output_img = argv[2];

    auto camera = construct_camera();

    Objects world;

    auto marble_texture = std::make_shared<MarbleTexture>(3., Vec3(0, 0, 1));
    auto turb_texture = std::make_shared<TurbulenceTexture>(2.);
    auto plain_texture =
        std::make_shared<ColorTexture>(Color::mix(Color::silver(), Color::white()));

    auto marble_material = std::make_shared<Lambertian>(marble_texture);
    auto medal_material = std::make_shared<Metal>(Color::brown(), 0.05);
    auto diffuse_material = std::make_shared<Lambertian>(Color::brown());
    auto silver_medal_mat = std::make_shared<Metal>(Color::gray(), 0.1);
    auto plain_material = std::make_shared<Lambertian>(plain_texture);

    world.add(std::make_shared<Sphere>(Point3(0, -1000, 0), 1000, silver_medal_mat));

    auto mesh = meshark::readGeometryMeshFromWavefrontObj(input_obj);

    auto mesh_obj = std::make_shared<TriangleMesh>(mesh.get(), medal_material, Point3(0, 3, 0), 6);
    world.add(mesh_obj);

    auto box = mesh_obj->bounding_box();
    auto center = Point3(
        0.5 * (box.x.min + box.x.max), 0.5 * (box.y.min + box.y.max),
        0.5 * (box.z.min + box.z.max));
    std::cout << std::format("adjusted camera.lookat to {}\n", center);
    camera.lookat = center;

    auto image = camera.render(world, true);

    auto file = std::ofstream(output_img, std::ios::binary | std::ios::out);
    dump(image, file);
    return 0;
}

}  // namespace demo::mesh
