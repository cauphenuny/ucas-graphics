#pragma once

#include <limits>
#include <random>

constexpr double infinity = std::numeric_limits<double>::infinity();
constexpr double pi = 3.1415926535897932385;

inline constexpr double degrees_to_radians(double degrees) { return degrees * pi / 180.0; }

inline double random_double_normal() {
    thread_local static std::mt19937 gen(std::random_device{}());
    std::normal_distribution<double> dist(0.0, 1.0);
    return dist(gen);
}

inline double random_double() {
    thread_local static std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(gen);
}

// NOTE: [min, max)
inline double random_double(double min, double max) {
    thread_local static std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<double> dist(min, max);
    return dist(gen);
}

// NOTE: [min, max]
inline int random_int(int min, int max) {
    thread_local static std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<int> dist(min, max);
    return dist(gen);
}

inline double trilinear_interplation(double c[2][2][2], double u, double v, double w) {
    double accum = 0.0;
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            for (int k = 0; k < 2; k++) {
                accum += (i * u + (1 - i) * (1 - u)) * (j * v + (1 - j) * (1 - v)) *
                         (k * w + (1 - k) * (1 - w)) * c[i][j][k];
            }
        }
    }
    return accum;
}

namespace traits {

template <typename T> struct CreateShared {
    template <typename... Args> static std::shared_ptr<T> create(Args... args) {
        return std::make_shared<T>(args...);
    }
};

}  // namespace traits
