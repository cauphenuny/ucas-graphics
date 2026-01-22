#pragma once

#include "utility.h"

#include <cmath>
#include <format>
#include <glm/glm.hpp>

class Vec3 {
    double d[3];

public:
    Vec3() = default;
    Vec3(double x, double y, double z) { d[0] = x, d[1] = y, d[2] = z; }
    Vec3(glm::vec3 v) { d[0] = v.x, d[1] = v.y, d[2] = v.z; }
    double x() const { return d[0]; }
    double y() const { return d[1]; }
    double z() const { return d[2]; }
    double operator[](int i) const { return d[i]; }
    double& operator[](int i) { return d[i]; }
    Vec3 operator+() const { return *this; }
    Vec3 operator-() const { return Vec3{-x(), -y(), -z()}; }
    Vec3& operator+=(const Vec3& v) {
        d[0] += v.x();
        d[1] += v.y();
        d[2] += v.z();
        return *this;
    }
    Vec3& operator-=(const Vec3& v) {
        d[0] -= v.x();
        d[1] -= v.y();
        d[2] -= v.z();
        return *this;
    }
    Vec3& operator*=(double t) {
        d[0] *= t;
        d[1] *= t;
        d[2] *= t;
        return *this;
    }
    Vec3& operator/=(double t) { return *this *= (1 / t); }
    friend Vec3 operator+(const Vec3& u, const Vec3& v);
    friend Vec3 operator-(const Vec3& u, const Vec3& v);
    friend Vec3 operator*(const Vec3& u, const Vec3& v);
    friend Vec3 operator*(const Vec3& v, double t);
    friend Vec3 operator*(double t, const Vec3& v);
    friend Vec3 operator/(const Vec3& v, double t);

    double sqrnorm() const { return x() * x() + y() * y() + z() * z(); }
    double norm() const { return std::sqrt(sqrnorm()); }
    Vec3 normalized() const;
    bool near_zero() const {
        auto s = 1e-8;
        return (std::fabs(x()) < s) && (std::fabs(y()) < s) && (std::fabs(z()) < s);
    }

    friend double dot(const Vec3& u, const Vec3& v);
    friend Vec3 cross(const Vec3& u, const Vec3& v);

    static Vec3 random() { return Vec3(random_double(), random_double(), random_double()); }
    static Vec3 random(double min, double max) {
        return Vec3(random_double(min, max), random_double(min, max), random_double(min, max));
    }

    static Vec3 random_unit() {
        while (true) {
            auto p = Vec3::random(-1, 1);
            auto sqrnorm = p.sqrnorm();
            if (1e-10 < sqrnorm && sqrnorm < 1) {
                return p * (1.0 / std::sqrt(sqrnorm));
            }
        }
    }
    static Vec3 random_on_hemisphere(const Vec3& normal) {
        Vec3 vec = random_unit();
        if (dot(vec, normal) > 0.0)
            return vec;
        else
            return -vec;
    }
    static Vec3 random_in_unit_disk() {
        while (true) {
            auto p = Vec3(random_double(-1, 1), random_double(-1, 1), 0);
            if (p.sqrnorm() < 1) {
                return p;
            }
        }
    }
    static Vec3 random_cosine_z() {  // NOTE: sampling PDF p(omega) = cos(theta) / pi
        auto r1 = random_double();
        auto r2 = random_double();
        auto phi = 2 * pi * r1;
        auto x = std::cos(phi) * std::sqrt(r2);
        auto y = std::sin(phi) * std::sqrt(r2);
        auto z = std::sqrt(1 - r2);
        return Vec3(x, y, z);
    }
    static Vec3 random_to_sphere(double radius, double distance_squared) {
        auto r1 = random_double();
        auto r2 = random_double();
        auto z = 1 + r2 * (std::sqrt(1 - radius * radius / distance_squared) - 1);

        auto phi = 2 * pi * r1;
        auto x = std::cos(phi) * std::sqrt(1 - z * z);
        auto y = std::sin(phi) * std::sqrt(1 - z * z);
        return Vec3(x, y, z);
    }
};

inline Vec3 operator+(const Vec3& u, const Vec3& v) {
    return Vec3(u.x() + v.x(), u.y() + v.y(), u.z() + v.z());
}

inline Vec3 operator-(const Vec3& u, const Vec3& v) {
    return Vec3(u.x() - v.x(), u.y() - v.y(), u.z() - v.z());
}

inline Vec3 operator*(const Vec3& u, const Vec3& v) {
    return Vec3(u.x() * v.x(), u.y() * v.y(), u.z() * v.z());
}

inline Vec3 operator*(double t, const Vec3& v) { return Vec3(t * v.x(), t * v.y(), t * v.z()); }

inline Vec3 operator*(const Vec3& v, double t) { return t * v; }

inline Vec3 operator/(const Vec3& v, double t) { return (1 / t) * v; }

inline double dot(const Vec3& u, const Vec3& v) {
    return u.x() * v.x() + u.y() * v.y() + u.z() * v.z();
}

inline Vec3 cross(const Vec3& u, const Vec3& v) {
    return Vec3(
        u.y() * v.z() - u.z() * v.y(), u.z() * v.x() - u.x() * v.z(),
        u.x() * v.y() - u.y() * v.x());
}

inline Vec3 reflect(const Vec3& v, const Vec3& n) { return v - 2 * dot(v, n) * n; }

inline Vec3 refract(const Vec3& uv, const Vec3& n, double etai_over_etat) {
    auto cos_theta = std::fmin(dot(-uv, n), 1.0);
    Vec3 r_out_perp = etai_over_etat * (uv + cos_theta * n);
    Vec3 r_out_parallel = -std::sqrt(std::fabs(1.0 - r_out_perp.sqrnorm())) * n;
    return r_out_perp + r_out_parallel;
}

inline Vec3 Vec3::normalized() const { return *this / norm(); }

using Point3 = Vec3;

template <typename CharT> struct std::formatter<Vec3, CharT> {
    std::formatter<double, CharT> component_formatter;

    constexpr auto parse(std::basic_format_parse_context<CharT>& ctx) {
        return component_formatter.parse(ctx);
    }

    template <typename FormatContext> auto format(const Vec3& v, FormatContext& ctx) const {
        auto it = ctx.out();
        it = std::format_to(it, "(");
        ctx.advance_to(it);
        it = component_formatter.format(v.x(), ctx);
        ctx.advance_to(it);
        it = std::format_to(it, ", ");
        ctx.advance_to(it);
        it = component_formatter.format(v.y(), ctx);
        ctx.advance_to(it);
        it = std::format_to(it, ", ");
        ctx.advance_to(it);
        it = component_formatter.format(v.z(), ctx);
        ctx.advance_to(it);
        it = std::format_to(it, ")");
        ctx.advance_to(it);
        return it;
    }
};

class Color : public Vec3 {
public:
    Color() = default;
    Color(double r, double g, double b) : Vec3(r, g, b) {}
    Color(Vec3 v) : Vec3(v) {}
    double r() const { return x(); }
    double g() const { return y(); }
    double b() const { return z(); }
    Color to_gamma() { return Color(std::sqrt(r()), std::sqrt(g()), std::sqrt(b())); }
    std::tuple<uint8_t, uint8_t, uint8_t> to_byte() const {
        auto red = std::clamp(r(), 0.0, 0.9999);
        auto green = std::clamp(g(), 0.0, 0.9999);
        auto blue = std::clamp(b(), 0.0, 0.9999);
        return std::make_tuple(int(256 * red), int(256 * green), int(256 * blue));
    }

    static Color red() { return Color(1.0, 0.2, 0.2); }
    static Color green() { return Color(0.2, 1.0, 0.2); }
    static Color blue() { return Color(0.2, 0.2, 1.0); }
    static Color orange() { return Color(1.0, 0.5, 0.0); }
    static Color white() { return Color(1.0, 1.0, 1.0); }
    static Color teal() { return Color(0.2, 0.8, 0.8); }
    static Color gray() { return Color(0.6, 0.6, 0.6); }
    static Color black() { return Color(0.0, 0.0, 0.0); }
    static Color yellow() { return Color(1.0, 1.0, 0.2); }
    static Color purple() { return Color(0.8, 0.2, 0.8); }
    static Color pink() { return Color(1.0, 0.2, 0.6); }
    static Color brown() { return Color(0.6, 0.4, 0.2); }
    static Color cyan() { return Color(0.2, 1.0, 1.0); }
    static Color lime() { return Color(0.2, 1.0, 0.2); }
    static Color magenta() { return Color(1.0, 0.2, 1.0); }
    static Color olive() { return Color(0.5, 0.5, 0.0); }
    static Color navy() { return Color(0.0, 0.0, 0.5); }
    static Color silver() { return Color(0.75, 0.75, 0.75); }
    static Color gold() { return Color(1.0, 0.84, 0.0); }
    static Color mix(Color c0, Color c1, double c1_weight = 0.5) {
        return (1.0 - c1_weight) * c0 + c1_weight * c1;
    }
};

template <typename CharT> struct std::formatter<Color, CharT> {
    std::formatter<double, CharT> double_formatter;
    std::formatter<int, CharT> byte_formatter;
    bool format_as_double = false;

    constexpr auto parse(std::basic_format_parse_context<CharT>& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end()) {
            format_as_double = true;
        } else {
            format_as_double = false;
        }
        return double_formatter.parse(ctx);
    }

    template <typename FormatContext> auto format(const Color& v, FormatContext& ctx) const {
        auto it = ctx.out();
        auto append_literal = [&](const char* literal) {
            it = std::format_to(it, literal);
            ctx.advance_to(it);
        };
        auto append_component = [&](auto& formatter, const auto& value) {
            it = formatter.format(value, ctx);
            ctx.advance_to(it);
        };

        append_literal("(");
        if (format_as_double) {
            append_component(double_formatter, v.x());
            append_literal(", ");
            append_component(double_formatter, v.y());
            append_literal(", ");
            append_component(double_formatter, v.z());
        } else {
            auto [red, green, blue] = v.to_byte();
            append_component(byte_formatter, int(red));
            append_literal(", ");
            append_component(byte_formatter, int(green));
            append_literal(", ");
            append_component(byte_formatter, int(blue));
        }
        append_literal(")");
        return it;
    }
};
