#pragma once
#include "canvas.hpp"
#include "coord.hpp"
#include "draw.hpp"
#include "entity.hpp"

#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <cmath>
#include <fmt/format.h>
#include <functional>
#include <glfw/glfw3.h>
#include <magic_enum/magic_enum.hpp>
#include <memory>
#include <optional>
#include <spdlog/spdlog.h>
#include <string>
#include <utility>
#include <vector>

namespace opengl {

namespace painting {

struct MenuItem {
    std::string label;
    std::function<void()> action;
    Vertex2d top_left{0.0, 0.0};
    Vertex2d bottom_right{0.0, 0.0};

    [[nodiscard]] bool contains(const Vertex2d& point) const {
        return point.x >= top_left.x && point.x <= bottom_right.x && point.y <= top_left.y &&
               point.y >= bottom_right.y;
    }
};

enum class MenuKind { None, Main, ShapeSpecific };

struct MenuState {
    MenuKind kind{MenuKind::None};
    bool visible{false};
    Vertex2d anchor{0.0, 0.0};
    double width{4.0};
    double item_height{0.8};
    double padding{0.2};
    std::vector<MenuItem> items;

    void layout() {
        double overall_height =
            items.empty() ? 0.0
                          : (static_cast<double>(items.size()) * (item_height + padding) - padding);
        double left = anchor.x - width * 0.5;
        double first_top = anchor.y + overall_height * 0.5;
        for (std::size_t i = 0; i < items.size(); ++i) {
            auto& item = items[i];
            double top = first_top - static_cast<double>(i) * (item_height + padding);
            double bottom = top - item_height;
            item.top_left = Vertex2d{left, top};
            item.bottom_right = Vertex2d{left + width, bottom};
        }
    }
};

struct MenuOverlayEntity;

struct MenuOverlay {
    const MenuState* state{nullptr};
    Color panel_color{mix("foreground", "background", 0.8)};
    Color border_color{"foreground"};
    Color text_color{"foreground"};
    Color item_color{"background"};
    using EntityType = MenuOverlayEntity;
};

struct MenuOverlayEntity : Entity {
    MenuOverlay config;
    MenuOverlayEntity(Canvas* canvas, MenuOverlay config);
    void draw() const override;
    std::string repr() const override;
};

inline MenuOverlayEntity::MenuOverlayEntity(Canvas* canvas, MenuOverlay config)
    : Entity(canvas), config(std::move(config)) {}

inline void MenuOverlayEntity::draw() const {
    if (!config.state || !config.state->visible) {
        return;
    }
    const auto& state = *config.state;
    if (state.items.empty()) {
        return;
    }
    double overall_height =
        state.items.empty()
            ? 0.0
            : (static_cast<double>(state.items.size()) * (state.item_height + state.padding) -
               state.padding);
    Vertex2d panel_center{state.anchor.x, state.anchor.y};
    draw::rect_filled(
        panel_center, state.width + state.padding * 2.0, overall_height + state.padding * 2.0,
        config.panel_color);
    draw::rect_outline(
        panel_center, state.width + state.padding * 2.0, overall_height + state.padding * 2.0,
        config.border_color, 0.5);

    for (const auto& item : state.items) {
        double rect_width = item.bottom_right.x - item.top_left.x;
        double rect_height = item.top_left.y - item.bottom_right.y;
        Vertex2d rect_center{
            item.top_left.x + rect_width * 0.5, item.bottom_right.y + rect_height * 0.5};
        draw::rect_filled(rect_center, rect_width, rect_height, config.item_color);
        draw::rect_outline(rect_center, rect_width, rect_height, config.border_color, 0.4);
        Vertex2d text_pos{item.top_left.x + 0.2, rect_center.y};
        draw::text(text_pos, item.label, config.text_color, 0.7);
    }
}

inline std::string MenuOverlayEntity::repr() const { return "MenuOverlay"; }

struct Painter : ActionHandler {
    enum class ShapeType { Polygon, Rectangle };
    enum class State { Idle, DrawPolygon, DrawRectangle };

    Painter() = default;

    void on_key(int key, int action) override {
        spdlog::info("painter received key event: key={}, action={}", key, action);
        if (action != GLFW_PRESS) {
            return;
        }
        if (key == GLFW_KEY_SPACE && state == State::Idle) {
            open_main_menu();
            return;
        }
        if (key == GLFW_KEY_ESCAPE) {
            if (menu_state.visible) {
                close_menu();
                return;
            }
            if (state == State::DrawPolygon) {
                finalize_polygon();
            } else if (state == State::DrawRectangle) {
                finalize_rectangle();
            }
            return;
        }
        if (key == GLFW_KEY_BACKSPACE) {
            undo_last_shape();
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
        Vertex2d world = cursor_to_world(xpos, ypos);
        last_cursor_world = world;
        if (menu_state.visible) {
            if (!handle_menu_click(world)) {
                close_menu();
            }
            return;
        }
        if (active_shape == ShapeType::Polygon) {
            add_polygon_point(world);
        } else {
            handle_rectangle_click(world);
        }
    }

    void on_mouse_move(double xpos, double ypos) override {
        if (!canvas) {
            return;
        }
        last_cursor_world = cursor_to_world(xpos, ypos);
        if (state == State::DrawPolygon) {
            update_polygon_preview(last_cursor_world);
        } else if (state == State::DrawRectangle) {
            update_rectangle_preview();
        }
    }

private:
    struct RectangleDraft {
        Vertex2d first;
        Vertex2d second;
    };

    State state{State::Idle};
    ShapeType active_shape{ShapeType::Polygon};
    ShapeType last_committed_shape{ShapeType::Polygon};

    std::vector<Vertex2d> polygon_points;
    std::vector<Vertex2d> committed_polygon;
    std::optional<RectangleDraft> committed_rectangle;

    std::vector<std::unique_ptr<LineEntity>> segments;
    std::unique_ptr<LineEntity> preview_segment;
    std::unique_ptr<RectangleEntity> preview_rectangle;

    std::optional<Vertex2d> rect_first_corner;
    std::optional<Vertex2d> rect_second_corner;

    MenuState menu_state;
    std::unique_ptr<MenuOverlayEntity> menu_layer;

    Vertex2d last_cursor_world{0.0, 0.0};

    std::vector<std::string> stroke_palette{"black",         "red",         "green",
                                            "blue",          "yellow",      "magenta",
                                            "cyan",          "bright_red",  "bright_green",
                                            "bright_yellow", "bright_blue", "bright_magenta",
                                            "bright_cyan"};
    std::size_t stroke_color_index{0};
    std::vector<double> stroke_width_options{0.5, 1.0, 2.0, 3.5, 5.0};
    std::size_t stroke_width_index{1};
    std::vector<std::optional<std::string>> fill_palette{
        std::nullopt,
        std::string{"black"},
        std::string{"red"},
        std::string{"green"},
        std::string{"blue"},
        std::string{"yellow"},
        std::string{"magenta"},
        std::string{"cyan"},
        std::string{"bright_red"},
        std::string{"bright_green"},
        std::string{"bright_yellow"},
        std::string{"bright_blue"},
        std::string{"bright_magenta"},
        std::string{"bright_cyan"}};
    std::size_t fill_color_index{0};
    std::vector<double> radius_options{0.0, 0.1, 0.3, 0.5, 1.0, 1.5};
    std::size_t corner_radius_index{0};

    struct HistoryEntry {
        std::unique_ptr<Entity> entity;
        int priority;
    };

    std::vector<HistoryEntry> drawn_entities;
    static constexpr int kCommittedPriorityStep = 10;
    static constexpr int kWorkingPriorityBase = 1000000;
    int next_priority{10000};
    int working_priority_counter{0};

    Color preview_color{mix("foreground", "background", 0.8)};

    [[nodiscard]] auto allocate_committed_priority() -> int {
        int priority = next_priority;
        next_priority += kCommittedPriorityStep;
        return priority;
    }

    [[nodiscard]] auto allocate_working_priority() -> int {
        return kWorkingPriorityBase + working_priority_counter++;
    }

    void push_committed_entity(std::unique_ptr<Entity> entity) {
        if (!entity) {
            return;
        }
        int priority = allocate_committed_priority();
        entity->set_priority(priority);
        drawn_entities.push_back(HistoryEntry{std::move(entity), priority});
    }

    void replace_top_entity(std::unique_ptr<Entity> entity) {
        if (!entity || drawn_entities.empty()) {
            return;
        }
        int priority = drawn_entities.back().priority;
        entity->set_priority(priority);
        drawn_entities.back().entity = std::move(entity);
    }

    void undo_last_shape() {
        if (drawn_entities.empty()) {
            spdlog::info("undo requested but history is empty");
            return;
        }
        drawn_entities.pop_back();
        committed_polygon.clear();
        committed_rectangle.reset();
        last_committed_shape = ShapeType::Polygon;
        close_menu();
    }

    void ensure_menu_layer() {
        if (menu_layer || !canvas) {
            return;
        }
        MenuOverlay overlay{
            .state = &menu_state,
        };
        menu_layer = canvas->draw(overlay);
        menu_layer->set_priority(200000);
    }

    void open_main_menu() {
        ensure_menu_layer();
        menu_state.kind = MenuKind::Main;
        menu_state.anchor = menu_anchor();
        menu_state.visible = true;
        menu_state.items = build_main_menu_items();
        menu_state.layout();
    }

    void open_shape_menu() {
        ensure_menu_layer();
        menu_state.kind = MenuKind::ShapeSpecific;
        menu_state.anchor = menu_anchor();
        menu_state.visible = true;
        menu_state.items = build_shape_menu_items();
        menu_state.layout();
    }

    void close_menu() {
        menu_state.visible = false;
        menu_state.items.clear();
        menu_state.kind = MenuKind::None;
    }

    bool handle_menu_click(const Vertex2d& world) {
        if (!menu_state.visible) {
            return false;
        }
        for (auto& item : menu_state.items) {
            if (item.contains(world)) {
                if (item.action) {
                    item.action();
                }
                refresh_menu_items();
                return true;
            }
        }
        return false;
    }

    void refresh_menu_items() {
        if (!menu_state.visible) {
            return;
        }
        if (menu_state.kind == MenuKind::Main) {
            menu_state.items = build_main_menu_items();
        } else if (menu_state.kind == MenuKind::ShapeSpecific) {
            menu_state.items = build_shape_menu_items();
        }
        menu_state.layout();
    }

    std::vector<MenuItem> build_main_menu_items() {
        std::vector<MenuItem> items;
        auto shape_label = fmt::format("Shape: {}", magic_enum::enum_name(active_shape));
        items.push_back(
            MenuItem{
                .label = shape_label,
                .action = [this]() { cycle_shape_type(); },
            });
        auto stroke_label = fmt::format("Stroke color: {}", stroke_palette[stroke_color_index]);
        items.push_back(
            MenuItem{
                .label = stroke_label,
                .action = [this]() { cycle_stroke_color(); },
            });
        auto width_label =
            fmt::format("Stroke width: {:.1f}", stroke_width_options[stroke_width_index]);
        items.push_back(
            MenuItem{
                .label = width_label,
                .action = [this]() { cycle_stroke_width(); },
            });
        return items;
    }

    std::vector<MenuItem> build_shape_menu_items() {
        std::vector<MenuItem> items;
        auto fill_label = fill_palette[fill_color_index].has_value()
                              ? fmt::format("Fill: {}", *fill_palette[fill_color_index])
                              : std::string{"Fill: none"};
        items.push_back(
            MenuItem{
                .label = fill_label,
                .action =
                    [this]() {
                        cycle_fill_color();
                        rebuild_committed_shape();
                    },
            });
        if (last_committed_shape == ShapeType::Rectangle) {
            auto radius_label =
                fmt::format("Corner radius: {:.2f}", radius_options[corner_radius_index]);
            items.push_back(
                MenuItem{
                    .label = radius_label,
                    .action =
                        [this]() {
                            cycle_corner_radius();
                            rebuild_committed_shape();
                        },
                });
        }
        return items;
    }

    void cycle_shape_type() {
        active_shape =
            (active_shape == ShapeType::Polygon) ? ShapeType::Rectangle : ShapeType::Polygon;
        state = State::Idle;
        polygon_points.clear();
        rect_first_corner.reset();
        rect_second_corner.reset();
        clear_segments();
        clear_polygon_preview();
        preview_rectangle.reset();
    }

    void cycle_stroke_color() {
        stroke_color_index = (stroke_color_index + 1) % stroke_palette.size();
        rebuild_committed_shape();
    }

    void cycle_stroke_width() {
        stroke_width_index = (stroke_width_index + 1) % stroke_width_options.size();
        rebuild_committed_shape();
    }

    void cycle_fill_color() { fill_color_index = (fill_color_index + 1) % fill_palette.size(); }

    void cycle_corner_radius() {
        corner_radius_index = (corner_radius_index + 1) % radius_options.size();
    }

    void add_polygon_point(const Vertex2d& point) {
        if (state == State::Idle) {
            polygon_points.clear();
            state = State::DrawPolygon;
        }
        polygon_points.push_back(point);
        if (polygon_points.size() >= 2) {
            add_segment(polygon_points[polygon_points.size() - 2], polygon_points.back());
        }
        clear_polygon_preview();
    }

    void add_segment(const Vertex2d& start, const Vertex2d& end) {
        Line seg{
            .start = start,
            .end = end,
            .color = current_stroke_color(),
            .stroke = current_stroke_width(),
        };
        auto segment_entity = canvas->draw(seg);
        segment_entity->set_priority(allocate_working_priority());
        segments.push_back(std::move(segment_entity));
    }

    void update_polygon_preview(const Vertex2d& current) {
        if (state != State::DrawPolygon || polygon_points.empty()) {
            clear_polygon_preview();
            return;
        }
        if (!preview_segment) {
            Line seg{
                .start = polygon_points.back(),
                .end = current,
                .color = preview_color,
                .stroke = current_stroke_width(),
            };
            preview_segment = canvas->draw(seg);
            preview_segment->set_priority(allocate_working_priority());
        } else {
            preview_segment->config.start = polygon_points.back();
            preview_segment->config.end = current;
        }
    }

    void clear_polygon_preview() { preview_segment.reset(); }

    void finalize_polygon() {
        if (state != State::DrawPolygon) {
            return;
        }
        if (polygon_points.size() < 3) {
            spdlog::warn("polygon needs at least 3 points");
            cancel_polygon();
            return;
        }
        add_segment(polygon_points.back(), polygon_points.front());
        committed_polygon = polygon_points;
        last_committed_shape = ShapeType::Polygon;
        rebuild_committed_shape(/*replace_existing=*/false);
        open_shape_menu();
        cancel_polygon();
    }

    void cancel_polygon() {
        polygon_points.clear();
        state = State::Idle;
        clear_polygon_preview();
        clear_segments();
    }

    void handle_rectangle_click(const Vertex2d& point) {
        if (state == State::Idle) {
            state = State::DrawRectangle;
            rect_first_corner.reset();
            rect_second_corner.reset();
            preview_rectangle.reset();
        }
        if (!rect_first_corner) {
            rect_first_corner = point;
            return;
        }
        rect_second_corner = point;
        update_rectangle_preview();
    }

    void update_rectangle_preview() {
        if (state != State::DrawRectangle || !rect_first_corner) {
            preview_rectangle.reset();
            return;
        }
        Vertex2d second = rect_second_corner.value_or(last_cursor_world);
        Vertex2d first = *rect_first_corner;
        Vertex2d center{(first.x + second.x) * 0.5, (first.y + second.y) * 0.5};
        double width = std::abs(second.x - first.x);
        double height = std::abs(second.y - first.y);
        Rectangle rect{
            .center = center,
            .width = width,
            .height = height,
            .corner_radius = 0.0,
            .color = preview_color,
            .fill_color = std::nullopt,
            .stroke = 1.0,
        };
        if (!preview_rectangle) {
            preview_rectangle = canvas->draw(rect);
            preview_rectangle->set_priority(allocate_working_priority());
        } else {
            preview_rectangle->config.center = center;
            preview_rectangle->config.width = width;
            preview_rectangle->config.height = height;
        }
    }

    void finalize_rectangle() {
        if (state != State::DrawRectangle) {
            return;
        }
        if (!rect_first_corner || !rect_second_corner) {
            spdlog::warn("rectangle needs two corners");
            cancel_rectangle();
            return;
        }
        committed_rectangle = RectangleDraft{*rect_first_corner, *rect_second_corner};
        last_committed_shape = ShapeType::Rectangle;
        rebuild_committed_shape(/*replace_existing=*/false);
        open_shape_menu();
        cancel_rectangle();
    }

    void cancel_rectangle() {
        rect_first_corner.reset();
        rect_second_corner.reset();
        preview_rectangle.reset();
        state = State::Idle;
    }

    void clear_segments() { segments.clear(); }

    void rebuild_committed_shape(bool replace_existing = true) {
        if (replace_existing && drawn_entities.empty()) {
            return;
        }
        auto entity = build_committed_entity();
        if (!entity) {
            return;
        }
        if (replace_existing) {
            replace_top_entity(std::move(entity));
        } else {
            push_committed_entity(std::move(entity));
        }
    }

    std::unique_ptr<Entity> build_committed_entity() {
        if (!canvas) {
            return nullptr;
        }
        if (last_committed_shape == ShapeType::Polygon) {
            if (committed_polygon.size() < 3) {
                return nullptr;
            }
            Polygon poly{
                .points = committed_polygon,
                .color = current_stroke_color(),
                .fill_color = current_fill_color(),
                .stroke = current_stroke_width(),
            };
            return canvas->draw(poly);
        }
        if (last_committed_shape == ShapeType::Rectangle && committed_rectangle) {
            Vertex2d first = committed_rectangle->first;
            Vertex2d second = committed_rectangle->second;
            Vertex2d center{(first.x + second.x) * 0.5, (first.y + second.y) * 0.5};
            double width = std::abs(second.x - first.x);
            double height = std::abs(second.y - first.y);
            Rectangle rect{
                .center = center,
                .width = width,
                .height = height,
                .corner_radius = current_corner_radius(),
                .color = current_stroke_color(),
                .fill_color = current_fill_color(),
                .stroke = current_stroke_width(),
            };
            return canvas->draw(rect);
        }
        return nullptr;
    }

    Vertex2d canvas_center() const {
        if (!canvas) {
            return Vertex2d{0.0, 0.0};
        }
        const auto& proj = canvas->params.projection;
        double cx = (proj.left + proj.right) * 0.5;
        double cy = (proj.top + proj.bottom) * 0.5;
        return Vertex2d{cx, cy};
    }

    Vertex2d menu_anchor() const { return canvas_center(); }

    Color current_stroke_color() const { return Color{stroke_palette[stroke_color_index]}; }

    double current_stroke_width() const { return stroke_width_options[stroke_width_index]; }

    std::optional<Color> current_fill_color() const {
        if (!fill_palette[fill_color_index]) {
            return std::nullopt;
        }
        return Color{*fill_palette[fill_color_index]};
    }

    double current_corner_radius() const { return radius_options[corner_radius_index]; }

    Vertex2d cursor_to_world(double xpos, double ypos) const {
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
