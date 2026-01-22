#pragma once

#include "bvh.h"
#include "hittable.h"
#include "material.h"
#include "shape.h"

#include <format>
#include <iostream>
#include <memory>
#include <meshark/geometry-mesh.h>
#include <meshark/mesh-io.h>

class TriangleMesh : public Hittable, public traits::CreateShared<TriangleMesh> {
    std::shared_ptr<Hittable> container;

    void build(
        meshark::GeometryMesh* mesh, std::shared_ptr<Material> mat, Point3 origin,
        Vec3 scale = Vec3(1, 1, 1)) {
        HittableList triangles;
        for (auto f : mesh->faces()) {
            auto h = f->halfEdge();
            auto v0 = scale * mesh->pos(h->tail) + origin;
            auto v1 = scale * mesh->pos(h->tip) + origin;
            auto v2 = scale * mesh->pos(h->next->tip) + origin;
            triangles.add(std::make_shared<Triangle>(v0, v1 - v0, v2 - v0, mat));
        }
        container = std::make_shared<BVHNode>(triangles);
        std::clog << std::format(
                         "Loaded mesh with {} triangles, bounding box: {}", mesh->numFaces(),
                         container->bounding_box())
                  << std::flush;
    }

public:
    TriangleMesh(
        meshark::GeometryMesh* mesh, std::shared_ptr<Material> mat, Point3 origin,
        Vec3 scale = Vec3(1, 1, 1)) {
        build(mesh, mat, origin, scale);
    }
    TriangleMesh(
        meshark::GeometryMesh* mesh, std::shared_ptr<Material> mat, Point3 origin,
        double max_length) {
        auto xrange = Interval::empty();
        auto yrange = Interval::empty();
        auto zrange = Interval::empty();
        for (auto f : mesh->faces()) {
            for (auto h : f->boundaryHalfEdges()) {
                auto p = mesh->pos(h->tail);
                xrange.min = std::fmin(xrange.min, p.x);
                xrange.max = std::fmax(xrange.max, p.x);
                yrange.min = std::fmin(yrange.min, p.y);
                yrange.max = std::fmax(yrange.max, p.y);
                zrange.min = std::fmin(zrange.min, p.z);
                zrange.max = std::fmax(zrange.max, p.z);
            }
        }
        auto length = std::fmax(xrange.size(), std::fmax(yrange.size(), zrange.size()));
        auto scale_factor = max_length / length;
        build(mesh, mat, origin, Vec3(scale_factor, scale_factor, scale_factor));
    }

    bool hit(const Ray& ray, Interval interval, HitResult& result) const override {
        return container->hit(ray, interval, result);
    }
    BoundingBox bounding_box() const override { return container->bounding_box(); }
};
