#pragma once

#include <cmath>
#include <format>

class Vec3 {
    double d[3];

public:
    Vec3() = default;
    Vec3(double x, double y, double z) { d[0] = x, d[1] = y, d[2] = z; }
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
    double sqrnorm() const { return x() * x() + y() * y() + z() * z(); }
    double norm() const { return std::sqrt(sqrnorm()); }
    Vec3 normalized() const;
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
    template <typename... Args> Color(Args... args) : Vec3(args...) {}
    double r() const { return x(); }
    double g() const { return y(); }
    double b() const { return z(); }
    std::tuple<uint8_t, uint8_t, uint8_t> to_byte() const {
        auto red = std::clamp(r(), 0.0, 0.9999);
        auto green = std::clamp(g(), 0.0, 0.9999);
        auto blue = std::clamp(b(), 0.0, 0.9999);
        return std::make_tuple(int(256 * red), int(256 * green), int(256 * blue));
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
