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
    cam.vfov = 30.0;
    cam.lookfrom = Point3(-20, 10, -5);
    cam.lookat = Point3(0, 1, 0);
    cam.vup = Vec3(0, 1, 0);
    cam.defocus_angle = 0.0;
    return cam;
}

inline int main(int argc, char** argv) {
    if (argc < 2) return 1;
    auto output_img = argv[1];
    auto input_obj = argc > 2 ? argv[2] : "assets/mesh/spot.obj";

    auto camera = construct_camera();

    HittableList world;

    auto marble_texture = MarbleTexture::create(3., Vec3(0, 0, 1));
    auto turb_texture = TurbulenceTexture::create(2.);
    auto plain_texture = ColorTexture::create(Color::mix(Color::silver(), Color::white()));

    auto marble_material = Lambertian::create(marble_texture);
    auto medal_material = Metal::create(Color::brown(), 0.2);
    auto diffuse_material = Lambertian::create(Color::brown());
    auto silver_medal_mat = Metal::create(Color::gray(), 0.05);
    auto plain_material = Lambertian::create(plain_texture);

    world.add(Sphere::create(Point3(0, -1000, 0), 1000, silver_medal_mat));

    auto mesh = meshark::readGeometryMeshFromWavefrontObj(input_obj);

    auto mesh_obj = TriangleMesh::create(mesh.get(), medal_material, Point3(0, 2.5, 0), 5);
    world.add(mesh_obj);

    auto box = mesh_obj->bounding_box();

    auto image = camera.render(world, true);

    auto file = std::ofstream(output_img, std::ios::binary | std::ios::out);
    dump(image, file);
    return 0;
}

}  // namespace demo::mesh
