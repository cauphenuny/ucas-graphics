#include "object/sphere.hpp"
#include "object/vec.hpp"

template <typename CoordT, typename RenderT, typename Container>
Vec3<RenderT> trace(
    const Vec3<CoordT>& ray_origin, const Vec3<CoordT>& ray_direction, const Container& objects,
    const Vec3<RenderT>& background_color, int depth) {
    CoordT tnear = std::numeric_limits<CoordT>::max();
    const Sphere<CoordT, RenderT>* nearest_object = nullptr;
    for (const auto& object : objects) {
        if (auto [hit, t0, t1] = object.intersect(ray_origin, ray_direction); hit) {
            if (t0 < 0) t0 = t1;
            if (t0 < tnear) {
                tnear = t0;
                nearest_object = &object;
            }
        }
    }

    if (!nearest_object) {
        return background_color;  // background color
    }

    Vec3<RenderT> surface_color = 0;

    Vec3<CoordT> phit = ray_origin + ray_direction * tnear;  // point of intersection
    Vec3<CoordT> nhit = phit - nearest_object->center;       // normal at the intersection point
    nhit.normalize();

    bool inside = false;
    if (ray_direction.dot(nhit) > 0) {
        nhit = -nhit;
        inside = true;
    }

    CoordT bias = 1e-4;  // add some bias to the point from which we will be tracing
                         // the reflection ray

    if (depth && (nearest_object->transparency || nearest_object->reflection)) {
        CoordT facing_ratio = -ray_direction.dot(nhit);
        RenderT fresnel_effect =
            mix(std::pow(1 - facing_ratio, 3), 1.0,
                0.1);  // change 0.1 to something else for different effect
        Vec3<CoordT> reflect_dir = ray_direction - nhit * 2 * ray_direction.dot(nhit);
        reflect_dir.normalize();
        Vec3<RenderT> reflection =
            trace(phit + nhit * bias, reflect_dir, objects, background_color, depth - 1);
        Vec3<RenderT> refraction = 0;

        if (nearest_object->transparency) {
            RenderT ior = 1.1;                     // index of refraction
            RenderT eta = inside ? ior : 1 / ior;  // are we inside or outside the surface?
            CoordT cosi = -nhit.dot(ray_direction);
            CoordT k = 1 - eta * eta * (1 - cosi * cosi);
            Vec3<CoordT> refract_dir = ray_direction * eta + nhit * (eta * cosi - std::sqrt(k));
            refract_dir.normalize();
            refraction =
                trace(phit - nhit * bias, refract_dir, objects, background_color, depth - 1);
        }
        surface_color = (reflection * fresnel_effect +
                         refraction * (1 - fresnel_effect) * nearest_object->transparency) *
                        nearest_object->surface_color;
    } else {
        for (auto& object : objects) {
            if (object.emission_color.norm() > 0) {
                Vec3<RenderT> transmission = 1;
                Vec3<CoordT> light_direction =
                    object.center - phit;  // direction from intersection point to light
                light_direction.normalize();
                for (auto& other_object : objects) {
                    if (&other_object == &object) continue;
                    if (auto [hit, t0, t1] =
                            other_object.intersect(phit + nhit * bias, light_direction);
                        hit) {
                        transmission = 0;
                        break;
                    }
                }
                surface_color += nearest_object->surface_color * transmission *
                                 std::max(RenderT(0), nhit.dot(light_direction)) *
                                 object.emission_color;
            }
        }
    }

    return surface_color + nearest_object->emission_color;
}

template <typename CoordT> struct Camera {
    unsigned width{640}, height{480};
    CoordT fov{30};
};

template <typename CoordT, typename RenderT, typename Container>
auto render(
    Camera<CoordT> camera, Container object, Vec3<RenderT> background_color, int max_depth) {
    std::vector<Vec3<RenderT>> image(camera.width * camera.height);
    CoordT inv_width = 1 / CoordT(camera.width);
    CoordT inv_height = 1 / CoordT(camera.height);
    CoordT aspect_ratio = CoordT(camera.width) / CoordT(camera.height);
    CoordT angle = std::tan(M_PI * 0.5 * camera.fov / 180.);

    for (unsigned y = 0; y < camera.height; ++y) {
        for (unsigned x = 0; x < camera.width; ++x) {
            CoordT xx = (2 * ((x + 0.5) * inv_width) - 1) * angle * aspect_ratio;
            CoordT yy = (1 - 2 * ((y + 0.5) * inv_height)) * angle;
            Vec3<CoordT> ray_direction(xx, yy, -1);
            ray_direction.normalize();
            image[y * camera.width + x] =
                trace(Vec3<CoordT>(0), ray_direction, object, background_color, max_depth);
        }
    }
    return image;
}
