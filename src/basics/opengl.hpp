#pragma once

#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <algorithm>
#include <cassert>
#include <cctype>
#include <fmt/format.h>
#include <glfw/glfw3.h>
#include <magic_enum/magic_enum.hpp>
#include <memory>
#include <set>
#include <spdlog/spdlog.h>
#include <string>
#include <sys/types.h>

namespace opengl {

struct Vertex2d {
    GLdouble x, y;
    Vertex2d(GLdouble x, GLdouble y) : x(x), y(y) {}
    template <typename T> Vertex2d(std::initializer_list<T> init) {
        assert(init.size() == 2);
        x = *init.begin();
        y = *(std::next(init.begin()));
    }
};

struct HexColor {
    unsigned int hex;
};

struct RGBColor {
    GLdouble red, green, blue, alpha{1.0};
    RGBColor(GLdouble r, GLdouble g, GLdouble b) : red(r), green(g), blue(b) {}
    RGBColor(HexColor hex);
    RGBColor(const std::string_view color_name);
    RGBColor(const char* color_name) : RGBColor(std::string_view{color_name}) {}
};

namespace themes {

enum class Color {
    BLACK = 0,  // background
    RED = 1,
    GREEN = 2,
    YELLOW = 3,
    BLUE = 4,
    MAGENTA = 5,
    CYAN = 6,
    WHITE = 7,  // foreground
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
constexpr Palette DEFAULT = {0x000000, 0xAA0000, 0x00AA00, 0xAA5500, 0x0000AA, 0xAA00AA,
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
    0x1e1e2e, 0xed8796, 0xa6da95, 0xeed49f, 0x8aadf4, 0xc6a0f6, 0x8bd5ca, 0xcad3f5,
    0x24273a, 0xf38ba8, 0xa6e3a1, 0xf9e2af, 0x89bffa, 0xcba6f7, 0x94e2d5, 0xeff1f5};

inline Palette current_theme = DEFAULT;

}  // namespace themes

inline RGBColor::RGBColor(HexColor hex) {
    red = ((hex.hex >> 16) & 0xFF) / 255.0;
    green = ((hex.hex >> 8) & 0xFF) / 255.0;
    blue = (hex.hex & 0xFF) / 255.0;
}

inline RGBColor::RGBColor(const std::string_view color_name) {
    using namespace themes;
    std::string name{color_name};
    if (name == "foreground") {
        name = "white";
    } else if (name == "background") {
        name = "black";
    }
    // convert to uppercase because magic_enum expects enum names like "WHITE", "BLACK"
    std::transform(
        name.begin(), name.end(), name.begin(), [](unsigned char c) { return std::toupper(c); });
    auto color_enum = magic_enum::enum_cast<themes::Color>(name);
    if (color_enum.has_value()) {
        auto color = color_enum.value();
        unsigned int hex = current_theme.palette[static_cast<int>(color)];
        *this = RGBColor(HexColor{hex});
    } else {
        throw std::runtime_error(fmt::format("Unknown color name: {}", color_name));
    }
}

}  // namespace opengl

namespace fmt {
template <> struct formatter<opengl::Vertex2d> {
    // parse is required but we don't support any format specifications
    constexpr auto parse(fmt::format_parse_context& ctx) -> decltype(ctx.begin()) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const opengl::Vertex2d& p, FormatContext& ctx) const -> decltype(ctx.out()) {
        return fmt::format_to(ctx.out(), "Vertex2d({}, {})", p.x, p.y);
    }
};

template <> struct formatter<opengl::RGBColor> {
    constexpr auto parse(fmt::format_parse_context& ctx) -> decltype(ctx.begin()) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const opengl::RGBColor& c, FormatContext& ctx) const -> decltype(ctx.out()) {
        return fmt::format_to(ctx.out(), "RGBColor({}, {}, {})", c.red, c.green, c.blue);
    }
};
}  // namespace fmt

namespace opengl::draw {

inline void line(Vertex2d start, Vertex2d end, RGBColor color = {0.0, 0.0, 0.0}) {
    glColor3d(color.red, color.green, color.blue);
    glBegin(GL_LINES);
    glVertex2d(start.x, start.y);
    glVertex2d(end.x, end.y);
    glEnd();
}

inline void triangle(Vertex2d p1, Vertex2d p2, Vertex2d p3, RGBColor color = {0.0, 0.0, 0.0}) {
    spdlog::info("Drawing triangle with vertices: {}, {}, {}", p1, p2, p3);
    glColor3d(color.red, color.green, color.blue);
    glBegin(GL_TRIANGLES);
    for (auto p : {p1, p2, p3}) {
        glVertex2d(p.x, p.y);
    }
    glEnd();
}

}  // namespace opengl::draw

namespace opengl {

struct Canvas;

struct Entity {
    virtual void draw() const = 0;
    virtual ~Entity();
    Canvas* container;
    Entity(Canvas* canvas);
};

struct LineEntity;
struct TriangleEntity;

struct Line {
    Vertex2d start;
    Vertex2d end;
    RGBColor color{"foreground"};
    using EntityType = LineEntity;
};

struct LineEntity : Entity {
    Line config;
    LineEntity(Canvas* canvas, Line config) : Entity(canvas), config(config) {}
    void draw() const override { draw::line(config.start, config.end, config.color); }
};

struct Triangle {
    Vertex2d p1;
    Vertex2d p2;
    Vertex2d p3;
    RGBColor color{0.0, 0.0, 0.0};
    using EntityType = TriangleEntity;
};

struct TriangleEntity : Entity {
    Triangle config;

    TriangleEntity(Canvas* canvas, Triangle config) : Entity(canvas), config(config) {}

    void draw() const override { draw::triangle(config.p1, config.p2, config.p3, config.color); }
};

struct CanvasParameters {
    std::string title;
    struct {
        int width{800}, height{600};
    } display_size;
    RGBColor background{"background"};
    struct {
        GLdouble left{-5}, right{5}, bottom{-5}, top{5};
        GLdouble zNear{5}, zFar{15};
    } projection;
    struct {
        GLdouble eyeX, eyeY, eyeZ;
        GLdouble centerX, centerY, centerZ;
        GLdouble upX, upY, upZ;
    } view_point;
};

struct Canvas {
    const CanvasParameters params_;
    GLFWwindow* window;
    std::set<Entity*> entities;

    Canvas(const CanvasParameters& params = CanvasParameters()) : params_(params) {
        this->window_init();
        this->init();
    }

    void window_init() {
        glfwInit();
        auto [w, h] = this->params_.display_size;
        spdlog::info("create GLFW window width: {}, height: {}", w, h);
        this->window = glfwCreateWindow(w, h, this->params_.title.c_str(), NULL, NULL);
        if (!window) {
            glfwTerminate();
            throw std::runtime_error("glfw init failed");
        }
    }

    void init() {
        auto& [title, display_size, bg, proj, view] = this->params_;
        glfwMakeContextCurrent(this->window);
        glClearColor(bg.red, bg.green, bg.blue, bg.alpha);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(proj.left, proj.right, proj.bottom, proj.top, proj.zNear, proj.zFar);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        gluLookAt(
            view.eyeX, view.eyeY, view.eyeZ, view.centerX, view.centerY, view.centerZ, view.upX,
            view.upY, view.upZ);
    }

    void spin() {
        while (!glfwWindowShouldClose(this->window)) {
            glClear(GL_COLOR_BUFFER_BIT);
            for (auto entity : entities) {
                entity->draw();
            }
            glfwSwapBuffers(window);
            glfwPollEvents();
        }
        glfwTerminate();
    }

    void add_entity(Entity* entity) { entities.insert(entity); }
    void delete_entity(Entity* entity);

    template <typename T> auto create(T config) {
        return std::make_unique<typename T::EntityType>(this, config);
    }
};

inline Entity::Entity(Canvas* canvas) : container(canvas) { this->container->add_entity(this); }
inline Entity::~Entity() { this->container->delete_entity(this); }

inline void Canvas::delete_entity(Entity* entity) {
    auto it = entities.find(entity);
    if (it != entities.end()) {
        entities.erase(it);
    } else {
        throw std::runtime_error(fmt::format("Entity {} not found", (void*)entity));
    }
}

}  // namespace opengl
