#pragma once

#include "vec.h"

#include <format>
#include <fstream>
#include <vector>

inline void dump(std::vector<Color> image, int width, int height, std::ofstream& ofs) {
    ofs << std::format("P3\n{} {}\n255\n", width, height);
    for (unsigned i = 0; i < width * height; ++i) {
        auto [r, g, b] = image[i].to_byte();
        ofs << std::format("{} {} {}\n", r, g, b);
    }
}
