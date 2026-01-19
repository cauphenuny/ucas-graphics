#include "bvh.h"
#include "camera.h"
#include "export.h"
#include "material.h"
#include "medium.h"
#include "mesh.h"
#include "shape.h"
#include "sphere.h"
#include "transform.h"

#include <fstream>
#include <meshark/mesh-io.h>
#include <optional>

namespace demo::final1 {

inline auto construct_camera() {
    Camera cam;
    cam.aspect_ratio = 1.0;
    cam.image_width = 800;
    cam.samples_per_pixel = 5000;
    cam.max_depth = 50;
    cam.vfov = 40.0;
    cam.lookfrom = Point3(478, 278, -600);
    cam.lookat = Point3(278, 278, 0);
    cam.vup = Vec3(0, 1, 0);
    cam.defocus_angle = 0.0;
    cam.background = Color::black();
    return cam;
}

inline auto floor() {
    Objects boxes;
    auto ground = Lambertian::create(Color(0.48, 0.83, 0.53));
    int boxes_per_side = 20;
    for (int i = 0; i < boxes_per_side; i++) {
        for (int j = 0; j < boxes_per_side; j++) {
            auto w = 100.0;
            auto x0 = -1000.0 + i * w;
            auto z0 = -1000.0 + j * w;
            auto y0 = 0.0;
            auto x1 = x0 + w;
            auto y1 = random_double(1, 101);
            auto z1 = z0 + w;

            boxes.add(Box::create(Point3(x0, y0, z0), Point3(x1, y1, z1), ground));
        }
    }
    return BVHNode::create(boxes);
}

inline auto light() {
    auto light_material = Light::create(Color::white() * 7);
    return Quadrilateral::create(
        Point3(-150, 0, -132), Vec3(300, 0, 0), Vec3(0, 0, 265), light_material);
}

inline auto moving_sphere() {
    auto sphere_material = Lambertian::create(Color(0.7, 0.3, 0.1));
    return Sphere::create(Point3(0, 0, 0), Point3(30, 0, 0), 50, sphere_material);
}

inline auto earth() {
    auto earth_texture = ImageTexture::create("earth.jpg");
    auto earth_material = Lambertian::create(earth_texture);
    return Sphere::create(Vec3(0, 0, 0), 80, earth_material);
}

inline auto marble_sphere() {
    auto marble_texture = MarbleTexture::create(0.2, Vec3(1, 1, 0));
    return Sphere::create(Point3(0, 0, 0), 70, Lambertian::create(marble_texture));
}

inline auto volumetic_fog() {
    auto boundary = Sphere::create(Vec3(0, 0, 0), 5000, Dielectric::create(1.5));
    return ConstantMedium::create(boundary, 0.00015, Color(1.0, 1.0, 1.0));
}

struct SubsurfaceSphere : Hittable, traits::CreateShared<SubsurfaceSphere> {
    Objects container;
    SubsurfaceSphere(Point3 center, double radius, double density, Color color) {
        auto boundary = Sphere::create(center, radius, Dielectric::create(1.5));
        auto content = ConstantMedium::create(boundary, density, color);
        container.add(boundary);
        container.add(content);
    }
    bool hit(const Ray& ray, Interval interval, HitResult& result) const override {
        return container.hit(ray, interval, result);
    }
    BoundingBox bounding_box() const override { return container.bounding_box(); }
};

inline auto subsurface_sphere() {
    return SubsurfaceSphere::create(Point3(0, 0, 0), 70, 0.2, Color(0.2, 0.4, 0.9));
}

inline auto glass_sphere() { return Sphere::create(Point3(0, 0, 0), 50, Dielectric::create(1.5)); }

inline auto
mesh(const char* path, double size, std::optional<Color> color, double density = 0.007) {
    auto mesh = meshark::readGeometryMeshFromWavefrontObj(path);
    auto metal = Metal::create(Color::silver(), 0.05);
    auto glass = Dielectric::create(1.5);
    auto boundary = TriangleMesh::create(mesh.get(), glass, Point3(0, 0, 0), size);
    auto container = Objects::create();
    container->add(boundary);
    if (color) {
        auto content = ConstantMedium::create(boundary, density, *color);
        container->add(content);
    }
    return container;
}

inline int main(int argc, char** argv) {
    if (argc < 2) return 1;
    bool smoke = false;
    if (argc > 2) {
        smoke = std::atoi(argv[2]) != 0;
    }

    Objects world;

    world.add(floor());
    world.add(light() | Translate(270., 500., 100.));
    world.add(moving_sphere() | Translate(400., 400., 200.));
    world.add(earth() | Translate(400., 200., 400.));
    world.add(marble_sphere() | Translate(220., 280., 300.));
    world.add(subsurface_sphere() | Translate(360., 150., 145.));
    world.add(glass_sphere() | Translate(260, 150, 45));
    world.add(
        mesh("assets/mesh/spot.obj", 150, Color::brown(), 0.06) | RotateY(90) | RotateZ(-30) |
        Translate(80, 250, 100));
    world.add(
        mesh("assets/mesh/torus.obj", 100, Color::red(), 0.1) | RotateX(80) | RotateZ(-30) |
        Translate(140, 140, -50));
    world.add(volumetic_fog());

    auto camera = construct_camera();
    auto image = camera.render(world, true);

    auto file = std::ofstream(argv[1], std::ios::binary | std::ios::out);
    dump(image, file);
    return 0;
}

}  // namespace demo::final1
