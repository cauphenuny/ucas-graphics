#pragma once

#include "camera.h"

#include <format>
#include <fstream>
#include <vector>

inline void dump(RenderResult image, std::ofstream& ofs) {
    ofs << std::format("P3\n{} {}\n255\n", image.width, image.height);
    for (unsigned i = 0; i < image.width * image.height; ++i) {
        auto [r, g, b] = image.data[i].to_gamma().to_byte();
        ofs << std::format("{} {} {}\n", r, g, b);
    }
}
