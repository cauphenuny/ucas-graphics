#pragma once

#include <cmath>
#include <format>

template <typename T> struct Vec3 {
    T x, y, z;
    Vec3() : x(T(0)), y(T(0)), z(T(0)) {}
    Vec3(T x, T y, T z) : x(x), y(y), z(z) {}
    Vec3(T x) : x(x), y(x), z(x) {}

    Vec3<T> operator+(const Vec3<T>& other) const {
        return Vec3<T>(x + other.x, y + other.y, z + other.z);
    }

    Vec3<T> operator-(const Vec3<T>& other) const {
        return Vec3<T>(x - other.x, y - other.y, z - other.z);
    }

    Vec3<T> operator-() const { return Vec3<T>(-x, -y, -z); }

    Vec3<T> operator*(const Vec3<T>& other) const {
        return Vec3<T>(x * other.x, y * other.y, z * other.z);
    }

    Vec3<T>& operator+=(const Vec3<T>& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    Vec3<T>& operator*=(const Vec3<T>& other) {
        x *= other.x;
        y *= other.y;
        z *= other.z;
        return *this;
    }

    T norm() const { return std::sqrt(x * x + y * y + z * z); }

    T sqrnorm() const { return x * x + y * y + z * z; }

    auto& normalize() {
        T n = norm();
        if (n > T(0)) {
            x /= n;
            y /= n;
            z /= n;
        }
        return *this;
    }

    T dot(const Vec3<T>& other) const { return x * other.x + y * other.y + z * other.z; }

    Vec3<T> cross(const Vec3<T>& other) const {
        return Vec3<T>(
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x);
    }
};

template <typename T, typename CharT> struct std::formatter<Vec3<T>, CharT> {
    // Parses format specifications of the form 'v' (for vector)
    constexpr auto parse(std::basic_format_parse_context<CharT>& ctx) { return ctx.begin(); }

    template <typename FormatContext> auto format(const Vec3<T>& v, FormatContext& ctx) const {
        return std::format_to(ctx.out(), "({}, {}, {})", v.x, v.y, v.z);
    }
};

template <typename T> T mix(T a, T b, T mix) { return b * mix + a * (T(1) - mix); }
