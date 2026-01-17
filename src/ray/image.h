#pragma once

#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#include "external/stb_image.h"

#include <format>
#include <iostream>
#include <string>

class Image {
    const int bytes_per_pixel = 3;  // force load as RGB
    float* fdata = nullptr;
    std::unique_ptr<unsigned char[]> bdata;  // Linear 8-bit pixel data
    int image_width = 0;                     // Loaded image width
    int image_height = 0;                    // Loaded image height
    int bytes_per_scanline = 0;

public:
    Image() = default;
    ~Image() { STBI_FREE(fdata); }

    Image(const char* filename) {
        auto name = std::string(filename);
        std::string dir = "assets/ray/";
        std::string prefix = "";

        for (int i = 0; i < 10; i++) {
            auto path = prefix + dir + name;
            if (load(path)) return;
            prefix = prefix + "../";
        }

        std::cerr << std::format("ERROR: Could not load image file '{}'", filename);
    }

    int width() const { return fdata ? image_width : 0; }
    int height() const { return fdata ? image_height : 0; }

    const unsigned char* data() const { return bdata.get(); }
    const unsigned char* data(int x, int y) const {
        static unsigned char magenta[] = {255, 0, 255};
        if (!bdata) return magenta;
        x = std::clamp(x, 0, image_width - 1);
        y = std::clamp(y, 0, image_height - 1);

        return bdata.get() + y * bytes_per_scanline + x * bytes_per_pixel;
    }

    bool load(const std::string& filename) {
        auto n = bytes_per_pixel;  // Dummy out parameter: original components per pixel
        fdata = stbi_loadf(filename.c_str(), &image_width, &image_height, &n, bytes_per_pixel);
        if (fdata == nullptr) return false;
        bytes_per_scanline = image_width * bytes_per_pixel;
        convert_to_bytes();
        return true;
    }

    static unsigned char float_to_byte(float value) {
        if (value <= 0.0) return 0;
        if (1.0 <= value) return 255;
        return static_cast<unsigned char>(256.0 * value);
    }

    void convert_to_bytes() {
        int total_bytes = image_width * image_height * bytes_per_pixel;
        bdata = std::make_unique<unsigned char[]>(total_bytes);
        for (int i = 0; i < total_bytes; i++) {
            bdata[i] = float_to_byte(fdata[i]);
        }
    }
};
