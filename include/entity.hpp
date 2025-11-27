#pragma once

#include "color.hpp"
#include "coord.hpp"
#include "draw.hpp"

#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <cassert>
#include <cctype>
#include <fmt/format.h>
#include <glfw/glfw3.h>
#include <magic_enum/magic_enum.hpp>
#include <spdlog/spdlog.h>
#include <string>
#include <sys/types.h>

namespace opengl {

struct Canvas;
struct EntityAttribute;

struct Entity {
    virtual void draw() const = 0;
    virtual std::string repr() const = 0;
    virtual ~Entity();
    Canvas* container;
    Entity(Canvas* canvas);
    auto attribute() const -> EntityAttribute;
    void set_priority(int) const;
};

struct LineEntity;
struct TriangleEntity;

struct Line {
    Vertex2d start;
    Vertex2d end;
    RGBColor color{"foreground"};
    double stroke{1.0};
    using EntityType = LineEntity;
};

struct LineEntity : Entity {
    Line config;
    LineEntity(Canvas* canvas, Line config) : Entity(canvas), config(config) {}
    void draw() const override {
        draw::line(config.start, config.end, config.color, config.stroke);
    }
    std::string repr() const override {
        return fmt::format(
            "Line(start={}, end={}, color={}, stroke={})", config.start, config.end, config.color,
            config.stroke);
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

struct CircleEntity;

struct Circle {
    Vertex2d center;
    double radius;
    RGBColor color{"foreground"};
    bool filled{true};
    using EntityType = CircleEntity;
};

struct CircleEntity : Entity {
    Circle config;
    CircleEntity(Canvas* canvas, Circle config) : Entity(canvas), config(config) {}
    void draw() const override {
        if (config.filled) {
            draw::circle_filled(config.center, config.radius, config.color);
        } else {
            draw::circle_outline(config.center, config.radius, config.color);
        }
    }
    std::string repr() const override {
        return fmt::format(
            "Circle(center={}, radius={}, color={}, filled={})", config.center, config.radius,
            config.color, config.filled);
    }
};

struct ArcEntity;
struct Arc {
    Vertex2d center;
    double radius;
    double start_deg;
    double sweep_deg;
    RGBColor color{"foreground"};
    using EntityType = ArcEntity;
};

struct ArcEntity : Entity {
    Arc config;
    ArcEntity(Canvas* canvas, Arc config) : Entity(canvas), config(config) {}
    void draw() const override {
        draw::arc_outline(
            config.center, config.radius, config.start_deg, config.sweep_deg, config.color);
    }
    std::string repr() const override {
        return fmt::format(
            "Arc(center={}, radius={}, start_deg={}, sweep_deg={}, color={})", config.center,
            config.radius, config.start_deg, config.sweep_deg, config.color);
    }
};

struct RectangleEntity;
struct Rectangle {
    Vertex2d center;
    double width;
    double height;
    bool is_round{false};
    double corner_radius{0.0};
    RGBColor color{"foreground"};
    bool filled{true};
    double stroke{1.0};
    using EntityType = RectangleEntity;
};

struct RectangleEntity : Entity {
    Rectangle config;
    RectangleEntity(Canvas* canvas, Rectangle config) : Entity(canvas), config(config) {}
    void draw() const override {
        if (config.filled) {
            if (config.is_round) {
                draw::rounded_rect_filled(
                    config.center, config.width, config.height, config.corner_radius, config.color);
            } else {
                draw::rect_filled(config.center, config.width, config.height, config.color);
            }
        } else {
            if (config.is_round) {
                draw::rounded_rect_outline(
                    config.center, config.width, config.height, config.corner_radius, config.color,
                    config.stroke);
            } else {
                draw::rect_outline(
                    config.center, config.width, config.height, config.color, config.stroke);
            }
        }
    }
    std::string repr() const override {
        return fmt::format(
            "Rectangle(center={}, width={}, height={}, is_round={}, corner_radius={}, color={}, "
            "filled={})",
            config.center, config.width, config.height, config.is_round, config.corner_radius,
            config.color, config.filled);
    }
};

}  // namespace opengl