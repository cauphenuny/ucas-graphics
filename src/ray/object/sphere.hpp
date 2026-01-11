#pragma once

#include "color.hpp"
#include "vec.hpp"

template <typename CoordT, typename RenderT> struct Sphere;

template <typename CoordT, typename RenderT> class SphereConf {
    Vec3<CoordT> m_center;
    CoordT m_radius;
    Vec3<RenderT> m_surface_color;
    Vec3<RenderT> m_emission_color;
    RenderT m_transparency;
    RenderT m_reflection;
    friend class Sphere<CoordT, RenderT>;

public:
    SphereConf()
        : m_center{}, m_radius{}, m_surface_color{}, m_emission_color{}, m_transparency{},
          m_reflection{} {}
    SphereConf& center(Vec3<CoordT> const& c) {
        m_center = c;
        return *this;
    }
    SphereConf& center(auto... args) {
        m_center = Vec3<CoordT>(args...);
        return *this;
    }
    SphereConf& radius(CoordT r) {
        m_radius = r;
        return *this;
    }
    SphereConf& surface_color(Vec3<RenderT> const& sc) {
        m_surface_color = sc;
        return *this;
    }
    SphereConf& emission_color(Vec3<RenderT> const& ec) {
        m_emission_color = ec;
        return *this;
    }
    SphereConf& surface_color(auto... args) {
        m_surface_color = Vec3<RenderT>(args...);
        return *this;
    }
    SphereConf& emission_color(auto... args) {
        m_emission_color = Vec3<RenderT>(args...);
        return *this;
    }
    SphereConf& transparency(RenderT t) {
        m_transparency = t;
        return *this;
    }
    SphereConf& reflection(RenderT r) {
        m_reflection = r;
        return *this;
    }
};

template <typename CoordT, typename RenderT> struct Sphere {
    using conf = SphereConf<CoordT, RenderT>;
    Vec3<CoordT> center;
    CoordT radius;
    Vec3<RenderT> surface_color;
    Vec3<RenderT> emission_color;
    RenderT transparency;
    RenderT reflection;
    Sphere(SphereConf<CoordT, RenderT> const& conf)
        : center(conf.m_center), radius(conf.m_radius), surface_color(conf.m_surface_color),
          emission_color(conf.m_emission_color), transparency(conf.m_transparency),
          reflection(conf.m_reflection) {}
    using Vec3C = Vec3<CoordT>;
    using Vec3R = Vec3<RenderT>;

    auto intersect(const Vec3C& origin, const Vec3C& direction) const {
        struct IntersectResult {
            bool hit;
            CoordT t0, t1;
        };
        CoordT t0, t1;  // solutions for t if the ray intersects

        // geometric solution
        Vec3C l = center - origin;
        CoordT tca = l.dot(direction);
        if (tca < 0) return IntersectResult{false, 0, 0};
        CoordT d2 = l.dot(l) - tca * tca;
        if (d2 > radius * radius) return IntersectResult{false, 0, 0};
        CoordT thc = std::sqrt(radius * radius - d2);
        t0 = tca - thc;
        t1 = tca + thc;

        return IntersectResult{true, t0, t1};
    }
};
