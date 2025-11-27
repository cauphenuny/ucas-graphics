#pragma once

#include <OpenGL/gl.h>
#include <OpenGL/gltypes.h>
#include <algorithm>
#include <fmt/format.h>
#include <glfw/glfw3.h>
#include <magic_enum/magic_enum.hpp>
#include <stdexcept>
#include <string>
#include <string_view>

namespace opengl {

struct HexColor {
    unsigned int hex;
};

struct Color {
    GLdouble red, green, blue, alpha{1.0};
    Color(GLubyte r, GLubyte g, GLubyte b, GLubyte a = 255)
        : red(r / 255.0), green(g / 255.0), blue(b / 255.0), alpha(a / 255.0) {}
    Color(GLdouble r, GLdouble g, GLdouble b, GLdouble a = 1.0)
        : red(r), green(g), blue(b), alpha(a) {}
    Color(HexColor hex);
    Color(const std::string_view color_name);
    Color(const char* color_name) : Color(std::string_view{color_name}) {}
};

namespace themes {

enum class ColorID {
    WHITE = 0,  // background
    RED = 1,
    GREEN = 2,
    YELLOW = 3,
    BLUE = 4,
    MAGENTA = 5,
    CYAN = 6,
    BLACK = 7,  // foreground
    BRIGHT_BLACK = 8,
    BRIGHT_RED = 9,
    BRIGHT_GREEN = 10,
    BRIGHT_YELLOW = 11,
    BRIGHT_BLUE = 12,
    BRIGHT_MAGENTA = 13,
    BRIGHT_CYAN = 14,
    BRIGHT_WHITE = 15,
};

struct Palette {
    unsigned int palette[18];
};

/*
Black (0)	#000000
Red (1)	#AA0000
Green (2)	#00AA00
Yellow (3)	#AA5500
Blue (4)	#0000AA
Magenta (5)	#AA00AA
Cyan (6)	#00AAAA
White (7)	#AAAAAA
ANSI 亮色（Bright）8 色
名称	HEX
Bright Black / Gray (8)	#555555
Bright Red (9)	#FF5555
Bright Green (10)	#55FF55
Bright Yellow (11)	#FFFF55
Bright Blue (12)	#5555FF
Bright Magenta (13)	#FF55FF
Bright Cyan (14)	#55FFFF
Bright White (15)	#FFFFFF
*/
constexpr Palette XTERM_DARK = {0x000000, 0xAA0000, 0x00AA00, 0xAA5500, 0x0000AA, 0xAA00AA,
                             0x00AAAA, 0xAAAAAA, 0x555555, 0xFF5555, 0x55FF55, 0xFFFF55,
                             0x5555FF, 0xFF55FF, 0x55FFFF, 0xFFFFFF};

/*
Black (0)	#24273a
Red (1)	#ed8796
Green (2)	#a6da95
Yellow (3)	#eed49f
Blue (4)	#8aadf4
Magenta (5)	#c6a0f6
Cyan (6)	#8bd5ca
White (7)	#cad3f5
ANSI 亮色（Bright）8 色
名称	HEX
Bright Black / Gray (8)	#363a4f
Bright Red (9)	#f38ba8
Bright Green (10)	#a6e3a1
Bright Yellow (11)	#f9e2af
Bright Blue (12)	#89bffa
Bright Magenta (13)	#cba6f7
Bright Cyan (14)	#94e2d5
Bright White (15)	#eff1f5
*/


constexpr Palette CATPPUCCIN = {
    0xcad3f5, 0xed8796, 0xa6da95, 0xeed49f, 0x8aadf4, 0xc6a0f6, 0x8bd5ca, 0x1e1e2e,
    0x363a4f, 0xf38ba8, 0xa6e3a1, 0xf9e2af, 0x89bffa, 0xcba6f7, 0x94e2d5, 0xeff1f5, };

constexpr Palette CATPPUCCIN_DARK = {
    0x1e1e2e, 0xed8796, 0xa6da95, 0xeed49f, 0x8aadf4, 0xc6a0f6, 0x8bd5ca, 0xcad3f5,
    0x24273a, 0xf38ba8, 0xa6e3a1, 0xf9e2af, 0x89bffa, 0xcba6f7, 0x94e2d5, 0xeff1f5 };

inline Palette current_theme = CATPPUCCIN;

inline Palette find(const std::string_view name) {
    if (name == "xterm-dark") {
        return XTERM_DARK;
    } else if (name == "catppuccin") {
        return CATPPUCCIN;
    } else if (name == "catppuccin-dark") {
        return CATPPUCCIN_DARK;
    } else {
        throw std::runtime_error(fmt::format("Unknown theme name: {}", name));
    }
}

}  // namespace themes

inline Color::Color(HexColor hex) {
    red = ((hex.hex >> 16) & 0xFF) / 255.0;
    green = ((hex.hex >> 8) & 0xFF) / 255.0;
    blue = (hex.hex & 0xFF) / 255.0;
}

inline Color::Color(const std::string_view color_name) {
    using namespace themes;
    std::string name{color_name};
    if (name == "foreground") {
        name = "black";
    } else if (name == "background") {
        name = "white";
    }
    // convert to uppercase because magic_enum expects enum names like "WHITE", "BLACK"
    std::transform(
        name.begin(), name.end(), name.begin(), [](unsigned char c) { return std::toupper(c); });
    auto color_enum = magic_enum::enum_cast<themes::ColorID>(name);
    if (color_enum.has_value()) {
        auto color = color_enum.value();
        unsigned int hex = current_theme.palette[static_cast<int>(color)];
        *this = Color(HexColor{hex});
    } else {
        throw std::runtime_error(fmt::format("Unknown color name: {}", color_name));
    }
}

inline auto mix(Color c1, Color c2, double t) -> Color {
    t = std::max(0.0, std::min(1.0, t));
    GLdouble r = (c1.red * (1.0 - t) + c2.red * t);
    GLdouble g = (c1.green * (1.0 - t) + c2.green * t);
    GLdouble b = (c1.blue * (1.0 - t) + c2.blue * t);
    GLdouble a = (c1.alpha * (1.0 - t) + c2.alpha * t);
    return Color{r, g, b, a};
}

}  // namespace opengl

namespace fmt {

template <> struct formatter<opengl::Color> {
    constexpr auto parse(fmt::format_parse_context& ctx) -> decltype(ctx.begin()) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const opengl::Color& c, FormatContext& ctx) const -> decltype(ctx.out()) {
        return fmt::format_to(
            ctx.out(), "RGBColor({}, {}, {})", (uint8_t)(c.red * 255), (uint8_t)(c.green * 255),
            (uint8_t)(c.blue * 255));
    }
};

}  // namespace fmt