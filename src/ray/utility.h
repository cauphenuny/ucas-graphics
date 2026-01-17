#pragma once

#include <limits>
#include <random>

constexpr double infinity = std::numeric_limits<double>::infinity();
constexpr double pi = 3.1415926535897932385;

inline constexpr double degrees_to_radians(double degrees) { return degrees * pi / 180.0; }

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
