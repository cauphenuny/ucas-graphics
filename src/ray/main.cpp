#include "export.h"
#include "object.h"
#include "ray.h"
#include "sphere.h"
#include "utility.h"
#include "vec.h"

#include <fstream>
#include <vector>

Color ray_color(const Ray& r, const Object& world) {
    auto center = Point3(0, 0, -1);
    HitResult result;
    if (world.hit(r, 0, infinity, result)) {
        return 0.5 * Color(result.normal + Vec3(1, 1, 1));
    }

    Vec3 unit_direction = r.direction().normalized();
    auto a = 0.5 * (unit_direction.y() + 1.0);
    return (1.0 - a) * Color(1., 1., 1.) + a * Color(0.5, 0.7, 1.0);
}

int main(int argc, char** argv) {
    if (argc < 2) return 1;
    auto expect_ratio = 16. / 9.;
    int image_width = 400;
    int image_height = std::max(static_cast<int>(image_width / expect_ratio), 1);
    auto ratio = (double)image_width / (double)image_height;

    ObjectSet world;
    world.add(std::make_shared<Sphere>(Point3(0, 0, -1), 0.5));
    world.add(std::make_shared<Sphere>(Point3(0, -100.5, -1), 100));

    auto focal_length = 1.0;
    auto viewport_height = 2.0;
    auto viewport_width = ratio * viewport_height;
    auto center = Point3(0., 0., 0.);

    auto viewport_u = Vec3(viewport_width, 0., 0.);
    auto viewport_v = Vec3(0., -viewport_height, 0.);
    auto pixel_delta_u = viewport_u / image_width;
    auto pixel_delta_v = viewport_v / image_height;

    auto viewport_upper_left = center - Vec3(0, 0, focal_length) - viewport_u / 2 - viewport_v / 2;
    auto pixel00_loc = viewport_upper_left + pixel_delta_u / 2 + pixel_delta_v / 2;

    auto image = std::vector<Color>(image_width * image_height);

    for (int j = 0; j < image_height; ++j) {
        for (int i = 0; i < image_width; ++i) {
            auto pixel_loc = pixel00_loc + i * pixel_delta_u + j * pixel_delta_v;
            Ray r(center, pixel_loc - center);
            image[j * image_width + i] = ray_color(r, world);
        }
    }

    auto file = std::ofstream(argv[1], std::ios::binary | std::ios::out);
    dump(image, image_width, image_height, file);
    return 0;
}
