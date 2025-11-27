#pragma once
#include "canvas.hpp"
#include "coord.hpp"
#include "entity.hpp"

#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <glfw/glfw3.h>
#include <memory>
#include <spdlog/spdlog.h>
#include <vector>

namespace opengl {

namespace painting {

struct Painter : ActionHandler {

    enum class State {
        IDLE,
        DRAW_POLY,
    };

    std::vector<Vertex2d> points;
    std::vector<std::unique_ptr<LineEntity>> segments;
    std::unique_ptr<LineEntity> preview_segment;
    State state{State::IDLE};
    Color stroke_color{"bright_red"};
    Color preview_color{"bright_yellow"};
    double stroke_width{2.0};

    Painter() = default;

    void on_key(int key, int action) override {
        spdlog::info("painter received key event: key={}, action={}", key, action);
        if (action != GLFW_PRESS) {
            return;
        }
        if (key == GLFW_KEY_ESCAPE) {
            close_polygon();
        }
    }

    void on_mouse_button(int button, int action, int mods) override {
        spdlog::info(
            "painter received mouse button event: button={}, action={}, mods={}", button, action,
            mods);
        if (!canvas) {
            return;
        }
        if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS) {
            return;
        }
        double xpos = 0.0, ypos = 0.0;
        glfwGetCursorPos(canvas->window, &xpos, &ypos);
        add_point(cursor_to_world(xpos, ypos));
    }

    void on_mouse_move(double xpos, double ypos) override {
        // spdlog::info("painter received mouse move event: xpos={}, ypos={}", xpos, ypos);
        if (!canvas) {
            return;
        }
        update_preview(cursor_to_world(xpos, ypos));
    }

private:
    void add_point(const Vertex2d& point) {
        if (state == State::IDLE) {
            points.clear();
            state = State::DRAW_POLY;
            spdlog::info(
                "from {} to {}", magic_enum::enum_name<State>(State::IDLE),
                magic_enum::enum_name<State>(State::DRAW_POLY));
        }
        spdlog::info("adding point to polygon: {}", point);
        points.push_back(point);
        if (points.size() >= 2) {
            add_segment(points[points.size() - 2], points.back());
        }
        clear_preview();
    }

    void add_segment(const Vertex2d& start, const Vertex2d& end) {
        Line seg{
            .start = start,
            .end = end,
            .color = stroke_color,
            .stroke = stroke_width,
        };
        segments.push_back(canvas->draw(seg));
    }

    void close_polygon() {
        spdlog::info("closing polygon");
        if (state != State::DRAW_POLY) {
            return;
        }
        if (points.size() >= 2) {
            add_segment(points.back(), points.front());
        }
        points.clear();
        state = State::IDLE;
        clear_preview();
    }

    void update_preview(const Vertex2d& current) {
        if (state != State::DRAW_POLY || points.empty()) {
            clear_preview();
            return;
        }
        if (!preview_segment) {
            Line seg{
                .start = points.back(),
                .end = current,
                .color = preview_color,
                .stroke = stroke_width,
            };
            preview_segment = canvas->draw(seg);
            preview_segment->set_priority(100000);
        } else {
            preview_segment->config.start = points.back();
            preview_segment->config.end = current;
        }
    }

    void clear_preview() { preview_segment.reset(); }

    auto cursor_to_world(double xpos, double ypos) const -> Vertex2d {
        if (!canvas) {
            return Vertex2d{0.0, 0.0};
        }
        const double width = static_cast<double>(canvas->params.display_size.width);
        const double height = static_cast<double>(canvas->params.display_size.height);
        const auto& proj = canvas->params.projection;
        double nx = width > 0.0 ? xpos / width : 0.0;
        double ny = height > 0.0 ? ypos / height : 0.0;
        double world_x = proj.left + nx * (proj.right - proj.left);
        double world_y = proj.top - ny * (proj.top - proj.bottom);
        return Vertex2d{world_x, world_y};
    }
};

}  // namespace painting

}  // namespace opengl
