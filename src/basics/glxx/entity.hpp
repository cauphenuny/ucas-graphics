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
#include <optional>
#include <spdlog/spdlog.h>
#include <string>
#include <sys/types.h>
#include <utility>
#include <vector>

namespace opengl {

template <typename T> auto optional_repr(const std::optional<T>& value) -> std::string {
    if (value) {
        return fmt::format("{}", *value);
    }
    return "nullopt";
}

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
struct PolylineEntity;

struct Line {
    Vertex2d start;
    Vertex2d end;
    Color color{"foreground"};
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
    Color color{"foreground"};  // stroke color
    std::optional<Color> fill_color;
    double stroke{1.0};
    using EntityType = TriangleEntity;
};

struct TriangleEntity : Entity {
    Triangle config;

    TriangleEntity(Canvas* canvas, Triangle config) : Entity(canvas), config(config) {}

    void draw() const override {
        if (config.fill_color) {
            draw::triangle(config.p1, config.p2, config.p3, *config.fill_color);
        }
        if (config.stroke > 0.0) {
            draw::triangle_outline(config.p1, config.p2, config.p3, config.color, config.stroke);
        }
    }
    std::string repr() const override {
        return fmt::format(
            "Triangle(p1={}, p2={}, p3={}, color={}, fill_color={}, stroke={})", config.p1,
            config.p2, config.p3, config.color, optional_repr(config.fill_color), config.stroke);
    }
};

struct CircleEntity;

struct Circle {
    Vertex2d center;
    double radius;
    Color color{"foreground"};  // stroke color
    std::optional<Color> fill_color;
    double stroke{1.0};
    using EntityType = CircleEntity;
};

struct CircleEntity : Entity {
    Circle config;
    CircleEntity(Canvas* canvas, Circle config) : Entity(canvas), config(config) {}
    void draw() const override {
        if (config.fill_color) {
            draw::circle_filled(config.center, config.radius, *config.fill_color);
        }
        if (config.stroke > 0.0) {
            draw::circle_outline(config.center, config.radius, config.color, 64, config.stroke);
        }
    }
    std::string repr() const override {
        return fmt::format(
            "Circle(center={}, radius={}, color={}, fill_color={}, stroke={})", config.center,
            config.radius, config.color, optional_repr(config.fill_color), config.stroke);
    }
};

struct ArcEntity;
struct Arc {
    Vertex2d center;
    double radius;
    double start_deg;
    double sweep_deg;
    Color color{"foreground"};
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
    std::optional<double> corner_radius{};
    Color color{"foreground"};  // stroke color
    std::optional<Color> fill_color = Color{"foreground"};
    double stroke{1.0};
    using EntityType = RectangleEntity;
};

struct RectangleEntity : Entity {
    Rectangle config;
    RectangleEntity(Canvas* canvas, Rectangle config) : Entity(canvas), config(config) {}
    void draw() const override {
        if (config.fill_color) {
            if (config.corner_radius) {
                draw::rounded_rect_filled(
                    config.center, config.width, config.height, *config.corner_radius,
                    *config.fill_color);
            } else {
                draw::rect_filled(config.center, config.width, config.height, *config.fill_color);
            }
        }
        if (config.stroke > 0.0) {
            if (config.corner_radius) {
                draw::rounded_rect_outline(
                    config.center, config.width, config.height, *config.corner_radius, config.color,
                    config.stroke);
            } else {
                draw::rect_outline(
                    config.center, config.width, config.height, config.color, config.stroke);
            }
        }
    }
    std::string repr() const override {
        return fmt::format(
            "Rectangle(center={}, width={}, height={}, corner_radius={}, color={}, fill_color={}, "
            "stroke={})",
            config.center, config.width, config.height, optional_repr(config.corner_radius),
            config.color, optional_repr(config.fill_color), config.stroke);
    }
};

struct PolygonEntity;

struct Polygon {
    std::vector<Vertex2d> points;
    Color color{"foreground"};
    std::optional<Color> fill_color{};
    double stroke{1.0};
    using EntityType = PolygonEntity;
};

struct PolygonEntity : Entity {
    Polygon config;
    PolygonEntity(Canvas* canvas, Polygon config) : Entity(canvas), config(std::move(config)) {}

    void draw() const override {
        if (config.points.size() < 2) {
            return;
        }
        if (config.fill_color && config.points.size() >= 3) {
            for (std::size_t i = 1; i + 1 < config.points.size(); ++i) {
                draw::triangle(
                    config.points[0], config.points[i], config.points[i + 1], *config.fill_color);
            }
        }
        if (config.stroke > 0.0) {
            draw::detail::draw_polyline(config.points, true, config.color, config.stroke);
        }
    }

    std::string repr() const override {
        return fmt::format(
            "Polygon(points={}, color={}, fill_color={}, stroke={})", config.points.size(),
            config.color, optional_repr(config.fill_color), config.stroke);
    }
};

struct Polyline {
    std::vector<Vertex2d> points;
    Color color{"foreground"};
    double stroke{1.0};
    using EntityType = PolylineEntity;
};

struct PolylineEntity : Entity {
    Polyline config;
    PolylineEntity(Canvas* canvas, Polyline config) : Entity(canvas), config(std::move(config)) {}

    void draw() const override {
        if (config.points.size() < 2 || config.stroke <= 0.0) {
            return;
        }
        draw::detail::draw_polyline(config.points, false, config.color, config.stroke);
    }

    std::string repr() const override {
        return fmt::format(
            "Polyline(points={}, color={}, stroke={})", config.points.size(), config.color,
            config.stroke);
    }
};

}  // namespace opengl