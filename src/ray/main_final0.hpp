#include "bvh.h"
#include "camera.h"
#include "export.h"
#include "hittable.h"
#include "material.h"
#include "sphere.h"
#include "vec.h"

#include <fstream>

namespace demo::final0 {

inline auto construct_world() {
    Objects world;
    auto checker = CheckerTexture::create(0.3, Color(0.2, 0.3, 0.1), Color(0.9, 0.9, 0.9));
    auto ground_material = Lambertian::create(checker);
    world.add(Sphere::create(Point3(0, -1000, 0), 1000, ground_material));

    for (int a = -11; a < 11; a++) {
        for (int b = -11; b < 11; b++) {
            auto choose_mat = random_double();
            Point3 center(a + 0.9 * random_double(), 0.2, b + 0.9 * random_double());
            auto center2 = center + Vec3(0, random_double(0, 0.85) * random_double(0, 0.85), 0);

            if (((center - Point3(-4, 0.2, 0)).norm() > 0.9) &&
                ((center - Point3(0, 0.2, 0)).norm() > 0.9) &&
                ((center - Point3(4, 0.2, 0)).norm() > 0.9)) {
                std::shared_ptr<Material> sphere_material;
                if (choose_mat < 0.6) {
                    auto albedo = Color::random() * Color::random();
                    sphere_material = Lambertian::create(albedo);
                    world.add(Sphere::create(center, center2, 0.2, sphere_material));
                } else if (choose_mat < 0.9) {
                    auto albedo = Color::random(0.5, 1);
                    auto fuzz = random_double(0, 0.5);
                    sphere_material = Metal::create(albedo, fuzz);
                    world.add(Sphere::create(center, center2, 0.2, sphere_material));
                } else {
                    sphere_material = Dielectric::create(1.5);
                    world.add(Sphere::create(center, center2, 0.2, sphere_material));
                }
            }
        }
    }

    auto material1 = Dielectric::create(1.5);
    world.add(Sphere::create(Point3(0, 1, 0), 1.0, material1));

    auto material2 = Lambertian::create(Color(0.4, 0.2, 0.1));
    world.add(Sphere::create(Point3(-4, 1, 0), 1.0, material2));

    auto material3 = Metal::create(Color(0.7, 0.6, 0.5), 0.0);
    world.add(Sphere::create(Point3(4, 1, 0), 1.0, material3));

    // return world;
    return BVHNode(world);
}

inline auto construct_camera() {
    Camera cam;
    cam.aspect_ratio = 16. / 9.;
    cam.image_width = 800;
    cam.samples_per_pixel = 200;
    cam.max_depth = 50;

    cam.vfov = 20;
    cam.lookfrom = Point3(13, 2, -3);
    cam.lookat = Point3(0, 0, 0);
    cam.vup = Vec3(0, 1, 0);

    cam.defocus_angle = 0.6;
    cam.focus_dist = 10.0;

    return cam;
}

inline int main(int argc, char** argv) {
    if (argc < 2) return 1;

    auto world = construct_world();
    auto camera = construct_camera();
    auto image = camera.render(world, true);

    auto file = std::ofstream(argv[1], std::ios::binary | std::ios::out);
    dump(image, file);
    return 0;
}

}  // namespace demo::final0
