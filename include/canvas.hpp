#pragma once

#include <GLUT/glut.h>
#include <OpenGL/gl.h>
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

#include "color.hpp"

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
    // spdlog::info("Drawing triangle with vertices: {}, {}, {}", p1, p2, p3);
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
    virtual std::string repr() const = 0;
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
    std::string repr() const override {
        return fmt::format(
            "Line(start={}, end={}, color={})", config.start, config.end, config.color);
    }
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
    std::string repr() const override {
        return fmt::format(
            "Triangle(p1={}, p2={}, p3={}, color={})", config.p1, config.p2, config.p3,
            config.color);
    }
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
        spdlog::info("creating GLFW window width: {}, height: {}", w, h);
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
        spdlog::info("entering main loop, entity count: {}", entities.size());
        while (!glfwWindowShouldClose(this->window)) {
            glClear(GL_COLOR_BUFFER_BIT);
            for (auto entity : entities) {
                entity->draw();
            }
            glfwSwapBuffers(window);
            glfwPollEvents();
        }
        spdlog::info("exit main loop");
        glfwTerminate();
    }

    void add_entity(Entity* entity);
    void delete_entity(Entity* entity);

    template <typename T> auto draw(T config) {
        auto entity = std::make_unique<typename T::EntityType>(this, config);
        spdlog::info("draw: {}, id={}", entity->repr(), (void*)entity.get());
        return entity;
    }

    ~Canvas() {
        for (auto entity : entities) {
            spdlog::warn("canvas destructing with remaining entity {}", (void*)entity);
            entity->container = nullptr;
        }
        glfwDestroyWindow(this->window);
        spdlog::info("GLFW window destroyed");
    }
};

inline Entity::Entity(Canvas* canvas) : container(canvas) { this->container->add_entity(this); }
inline Entity::~Entity() { if (this->container) this->container->delete_entity(this); }

inline void Canvas::add_entity(Entity* entity) {
    entities.insert(entity);
}
inline void Canvas::delete_entity(Entity* entity) {
    auto it = entities.find(entity);
    if (it != entities.end()) {
        entities.erase(it);
    } else {
        throw std::runtime_error(fmt::format("Entity {} not found", (void*)entity));
    }
    spdlog::info("detached entity {} from canvas {}", (void*)entity, (void*)this);
}

}  // namespace opengl

namespace fmt {

template <> struct formatter<opengl::Entity*> {
    constexpr auto parse(fmt::format_parse_context& ctx) -> decltype(ctx.begin()) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const opengl::Entity* c, FormatContext& ctx) const -> decltype(ctx.out()) {
        return fmt::format_to(ctx.out(), "{}", c->repr());
    }
};

}