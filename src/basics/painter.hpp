#pragma once
#include "canvas.hpp"
#include "coord.hpp"
#include "drafts.hpp"
#include "draw.hpp"
#include "entity.hpp"

#include <GLUT/glut.h>
#include <OpenGL/gl.h>
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
    Painter() = default;

    void on_key(int key, int action) override {
        spdlog::info("painter received key event: key={}, action={}", key, action);
        if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
            open_main_menu();
            return;
        }
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            if (menu_state.visible) {
                close_menu();
                return;
            }
        }
        if (key == GLFW_KEY_BACKSPACE && action == GLFW_PRESS) {
            undo_last_shape();
            return;
        }
        ensure_current_draft();
        if (current_draft) {
            current_draft->on_key(key, action);
        }
    }

    void on_mouse_button(int button, int action, int mods) override {
        spdlog::info(
            "painter received mouse button event: button={}, action={}, mods={}", button, action,
            mods);
        if (!canvas) {
            return;
        }
        if (action != GLFW_PRESS) {
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
        ensure_current_draft();
        if (current_draft) {
            current_draft->on_mouse_button(button, action, mods, world);
        }
    }

    void on_mouse_move(double xpos, double ypos) override {
        if (!canvas) {
            return;
        }
        last_cursor_world = cursor_to_world(xpos, ypos);
        ensure_current_draft();
        if (current_draft) {
            current_draft->on_mouse_move(last_cursor_world);
        }
    }

private:
    struct HistoryEntry {
        std::unique_ptr<Entity> entity;
        int priority;
        ShapeType shape;
        std::function<std::unique_ptr<Entity>(Canvas*, const DraftStyle&)> rebuild;
    };

    ShapeType active_shape{ShapeType::Polygon};
    std::unique_ptr<Draft> current_draft;

    std::vector<HistoryEntry> drawn_entities;

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

    void ensure_current_draft() {
        if (!canvas) {
            return;
        }
        if (!current_draft) {
            current_draft = make_draft(active_shape);
        }
    }

    void reset_current_draft() {
        current_draft.reset();
        ensure_current_draft();
        refresh_active_draft_style();
    }

    std::unique_ptr<Draft> make_draft(ShapeType type) {
        if (!canvas) {
            return nullptr;
        }
        DraftContext ctx = make_draft_context();
        switch (type) {
            case ShapeType::Polygon: return std::make_unique<PolygonDraft>(std::move(ctx));
            case ShapeType::Rectangle: return std::make_unique<RectangleDraft>(std::move(ctx));
            case ShapeType::Line: return std::make_unique<LineDraft>(std::move(ctx));
            case ShapeType::Circle: return std::make_unique<CircleDraft>(std::move(ctx));
        }
        return nullptr;
    }

    DraftContext make_draft_context() {
        DraftContext ctx;
        ctx.canvas = canvas;
        ctx.preview_color = preview_color;
        ctx.style_provider = [this]() { return current_style(); };
        ctx.commit_callback = [this](DraftCommit commit_info) {
            handle_draft_commit(std::move(commit_info));
        };
        ctx.working_priority_allocator = [this]() { return allocate_working_priority(); };
        return ctx;
    }

    void handle_draft_commit(DraftCommit commit_info) {
        if (!commit_info.entity) {
            return;
        }
        int priority = allocate_committed_priority();
        commit_info.entity->set_priority(priority);
        drawn_entities.push_back(
            HistoryEntry{
                .entity = std::move(commit_info.entity),
                .priority = priority,
                .shape = commit_info.shape_type,
                .rebuild = std::move(commit_info.rebuild),
            });
        open_shape_menu();
    }

    void refresh_active_draft_style() {
        if (current_draft) {
            current_draft->refresh_style();
        }
    }

    DraftStyle current_style() const {
        return DraftStyle{
            .stroke_color = current_stroke_color(),
            .stroke_width = current_stroke_width(),
            .fill_color = current_fill_color(),
            .corner_radius = current_corner_radius(),
        };
    }

    void rebuild_last_entity() {
        if (drawn_entities.empty() || !canvas) {
            return;
        }
        auto& entry = drawn_entities.back();
        if (!entry.rebuild) {
            return;
        }
        auto entity = entry.rebuild(canvas, current_style());
        if (!entity) {
            return;
        }
        entity->set_priority(entry.priority);
        entry.entity = std::move(entity);
    }

    void undo_last_shape() {
        if (drawn_entities.empty()) {
            spdlog::info("undo requested but history is empty");
            return;
        }
        drawn_entities.pop_back();
        refresh_menu_items();
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
        items.push_back(MenuItem{.label = shape_label, .action = [this]() { cycle_shape_type(); }});
        auto stroke_label = fmt::format("Stroke color: {}", stroke_palette[stroke_color_index]);
        items.push_back(
            MenuItem{.label = stroke_label, .action = [this]() { cycle_stroke_color(); }});
        auto width_label =
            fmt::format("Stroke width: {:.1f}", stroke_width_options[stroke_width_index]);
        items.push_back(
            MenuItem{.label = width_label, .action = [this]() { cycle_stroke_width(); }});
        return items;
    }

    std::vector<MenuItem> build_shape_menu_items() {
        std::vector<MenuItem> items;
        auto shape = last_committed_shape();
        if (!shape) {
            items.push_back(MenuItem{.label = "No committed shape", .action = []() {}});
            return items;
        }
        if (shape_supports_fill(*shape)) {
            auto fill_label = fill_palette[fill_color_index].has_value()
                                  ? fmt::format("Fill: {}", *fill_palette[fill_color_index])
                                  : std::string{"Fill: none"};
            items.push_back(MenuItem{.label = fill_label, .action = [this]() {
                                         cycle_fill_color();
                                         rebuild_last_entity();
                                         refresh_menu_items();
                                     }});
        }
        if (*shape == ShapeType::Rectangle) {
            auto radius_label =
                fmt::format("Corner radius: {:.2f}", radius_options[corner_radius_index]);
            items.push_back(MenuItem{.label = radius_label, .action = [this]() {
                                         cycle_corner_radius();
                                         rebuild_last_entity();
                                         refresh_menu_items();
                                     }});
        }
        if (items.empty()) {
            items.push_back(MenuItem{.label = "No extra options", .action = []() {}});
        }
        return items;
    }

    std::optional<ShapeType> last_committed_shape() const {
        if (drawn_entities.empty()) {
            return std::nullopt;
        }
        return drawn_entities.back().shape;
    }

    bool shape_supports_fill(ShapeType shape) const {
        return shape == ShapeType::Polygon || shape == ShapeType::Rectangle ||
               shape == ShapeType::Circle;
    }

    void cycle_shape_type() {
        switch (active_shape) {
            case ShapeType::Polygon: active_shape = ShapeType::Rectangle; break;
            case ShapeType::Rectangle: active_shape = ShapeType::Line; break;
            case ShapeType::Line: active_shape = ShapeType::Circle; break;
            case ShapeType::Circle: active_shape = ShapeType::Polygon; break;
        }
        reset_current_draft();
        refresh_menu_items();
    }

    void cycle_stroke_color() {
        stroke_color_index = (stroke_color_index + 1) % stroke_palette.size();
        rebuild_last_entity();
        refresh_active_draft_style();
        refresh_menu_items();
    }

    void cycle_stroke_width() {
        stroke_width_index = (stroke_width_index + 1) % stroke_width_options.size();
        rebuild_last_entity();
        refresh_active_draft_style();
        refresh_menu_items();
    }

    void cycle_fill_color() { fill_color_index = (fill_color_index + 1) % fill_palette.size(); }

    void cycle_corner_radius() {
        corner_radius_index = (corner_radius_index + 1) % radius_options.size();
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
