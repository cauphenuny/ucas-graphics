#pragma once

#include "canvas.hpp"
#include "entity.hpp"

#include <cmath>
#include <functional>
#include <glfw/glfw3.h>
#include <memory>
#include <optional>
#include <tuple>
#include <utility>
#include <vector>

namespace opengl {

namespace painting {

enum class ShapeType { Line, Rectangle, Polygon, Circle, Polyline };

struct DraftStyle {
    Color stroke_color{"foreground"};
    double stroke_width{1.0};
    std::optional<Color> fill_color{};
    double corner_radius{0.0};
};

struct DraftCommit {
    std::unique_ptr<Entity> entity;
    std::function<std::unique_ptr<Entity>(Canvas*, const DraftStyle&)> rebuild;
    ShapeType shape_type{ShapeType::Polygon};
};

struct DraftContext {
    Canvas* canvas{nullptr};
    Color preview_color{mix("foreground", "background", 0.8)};
    std::function<DraftStyle()> style_provider;
    std::function<void(DraftCommit)> commit_callback;
    std::function<int()> working_priority_allocator;
};

class Draft {
public:
    explicit Draft(DraftContext ctx) : ctx_(std::move(ctx)) {}
    virtual ~Draft() = default;

    virtual std::string name() const = 0;
    virtual void on_mouse_button(int button, int action, int mods, const Vertex2d& world) = 0;
    virtual void on_mouse_move(const Vertex2d& world) = 0;
    virtual void on_key(int key, int action) {}
    virtual void refresh_style() {}
    virtual void reset() = 0;

protected:
    [[nodiscard]] DraftStyle current_style() const {
        if (ctx_.style_provider) {
            return ctx_.style_provider();
        }
        return DraftStyle{};
    }

    [[nodiscard]] Color preview_color() const { return ctx_.preview_color; }

    [[nodiscard]] Canvas* canvas() const { return ctx_.canvas; }

    [[nodiscard]] int allocate_working_priority() const {
        if (ctx_.working_priority_allocator) {
            return ctx_.working_priority_allocator();
        }
        return 0;
    }

    void commit(DraftCommit commit_info) {
        if (ctx_.commit_callback) {
            ctx_.commit_callback(std::move(commit_info));
        }
    }

private:
    DraftContext ctx_;
};

class LineDraft : public Draft {
public:
    explicit LineDraft(DraftContext ctx) : Draft(std::move(ctx)) {}

    std::string name() const override { return "Line"; }

    void on_mouse_button(int button, int action, int /*mods*/, const Vertex2d& world) override {
        if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS) {
            return;
        }
        if (!first_point_) {
            first_point_ = world;
            update_preview(world);
            return;
        }
        auto start = *first_point_;
        auto end = world;
        reset();
        commit(make_commit(start, end));
    }

    void on_mouse_move(const Vertex2d& world) override {
        if (!first_point_) {
            return;
        }
        update_preview(world);
    }

    void on_key(int key, int action) override {
        if (action != GLFW_PRESS) {
            return;
        }
        if (key == GLFW_KEY_ESCAPE) {
            reset();
        }
    }

    void refresh_style() override {
        if (preview_line_) {
            preview_line_->config.color = preview_color();
            preview_line_->config.stroke = current_style().stroke_width;
        }
    }

    void reset() override {
        first_point_.reset();
        preview_line_.reset();
    }

private:
    std::optional<Vertex2d> first_point_;
    std::unique_ptr<LineEntity> preview_line_;

    void ensure_preview_line() {
        if (preview_line_ || !canvas()) {
            return;
        }
        Line line{
            .start = first_point_.value_or(Vertex2d{0.0, 0.0}),
            .end = first_point_.value_or(Vertex2d{0.0, 0.0}),
            .color = preview_color(),
            .stroke = current_style().stroke_width,
        };
        preview_line_ = canvas()->draw(line);
        preview_line_->set_priority(allocate_working_priority());
    }

    void update_preview(const Vertex2d& world) {
        ensure_preview_line();
        if (!preview_line_) {
            return;
        }
        if (first_point_) {
            preview_line_->config.start = *first_point_;
        }
        preview_line_->config.end = world;
    }

    DraftCommit make_commit(const Vertex2d& start, const Vertex2d& end) {
        if (!canvas()) {
            return {};
        }
        auto style = current_style();
        Line config{
            .start = start,
            .end = end,
            .color = style.stroke_color,
            .stroke = style.stroke_width,
        };
        auto entity = canvas()->draw(config);
        auto points = std::make_pair(start, end);
        auto rebuild = [points](Canvas* canvas, const DraftStyle& s) {
            Line cfg{
                .start = points.first,
                .end = points.second,
                .color = s.stroke_color,
                .stroke = s.stroke_width,
            };
            return canvas->draw(cfg);
        };
        return DraftCommit{
            .entity = std::move(entity),
            .rebuild = rebuild,
            .shape_type = ShapeType::Line,
        };
    }
};

class RectangleDraft : public Draft {
public:
    explicit RectangleDraft(DraftContext ctx) : Draft(std::move(ctx)) {}

    std::string name() const override { return "Rectangle"; }

    void on_mouse_button(int button, int action, int /*mods*/, const Vertex2d& world) override {
        if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS) {
            return;
        }
        if (!first_corner_) {
            first_corner_ = world;
            return;
        }
        auto start = *first_corner_;
        auto end = world;
        reset();
        commit(make_commit(start, end));
    }

    void on_mouse_move(const Vertex2d& world) override {
        if (!first_corner_) {
            return;
        }
        update_preview(world);
    }

    void on_key(int key, int action) override {
        if (action != GLFW_PRESS) {
            return;
        }
        if (key == GLFW_KEY_ESCAPE) {
            reset();
        }
    }

    void refresh_style() override {
        if (preview_rectangle_) {
            auto style = current_style();
            preview_rectangle_->config.color = preview_color();
            preview_rectangle_->config.stroke = style.stroke_width;
            preview_rectangle_->config.corner_radius =
                style.corner_radius > 0.0 ? std::optional<double>(style.corner_radius)
                                          : std::nullopt;
        }
    }

    void reset() override {
        first_corner_.reset();
        preview_rectangle_.reset();
    }

private:
    std::optional<Vertex2d> first_corner_;
    std::unique_ptr<RectangleEntity> preview_rectangle_;

    static auto geometry(const Vertex2d& a, const Vertex2d& b)
        -> std::tuple<Vertex2d, double, double> {
        Vertex2d center{(a.x + b.x) * 0.5, (a.y + b.y) * 0.5};
        double width = std::abs(a.x - b.x);
        double height = std::abs(a.y - b.y);
        return {center, width, height};
    }

    void update_preview(const Vertex2d& world) {
        if (!canvas() || !first_corner_) {
            return;
        }
        auto style = current_style();
        auto [center, width, height] = geometry(*first_corner_, world);
        std::optional<double> radius =
            style.corner_radius > 0.0 ? std::optional<double>(style.corner_radius) : std::nullopt;
        Rectangle rect{
            .center = center,
            .width = width,
            .height = height,
            .corner_radius = radius,
            .color = preview_color(),
            .fill_color = std::nullopt,
            .stroke = style.stroke_width,
        };
        if (!preview_rectangle_) {
            preview_rectangle_ = canvas()->draw(rect);
            preview_rectangle_->set_priority(allocate_working_priority());
        } else {
            preview_rectangle_->config = rect;
        }
    }

    DraftCommit make_commit(const Vertex2d& a, const Vertex2d& b) {
        if (!canvas()) {
            return {};
        }
        auto style = current_style();
        auto [center, width, height] = geometry(a, b);
        std::optional<double> radius =
            style.corner_radius > 0.0 ? std::optional<double>(style.corner_radius) : std::nullopt;
        Rectangle rect{
            .center = center,
            .width = width,
            .height = height,
            .corner_radius = radius,
            .color = style.stroke_color,
            .fill_color = style.fill_color,
            .stroke = style.stroke_width,
        };
        auto entity = canvas()->draw(rect);
        auto corners = std::make_pair(a, b);
        auto rebuild = [corners](Canvas* canvas, const DraftStyle& style) {
            Vertex2d center{
                (corners.first.x + corners.second.x) * 0.5,
                (corners.first.y + corners.second.y) * 0.5};
            double width = std::abs(corners.first.x - corners.second.x);
            double height = std::abs(corners.first.y - corners.second.y);
            std::optional<double> radius = style.corner_radius > 0.0
                                               ? std::optional<double>(style.corner_radius)
                                               : std::nullopt;
            Rectangle cfg{
                .center = center,
                .width = width,
                .height = height,
                .corner_radius = radius,
                .color = style.stroke_color,
                .fill_color = style.fill_color,
                .stroke = style.stroke_width,
            };
            return canvas->draw(cfg);
        };
        return DraftCommit{
            .entity = std::move(entity),
            .rebuild = rebuild,
            .shape_type = ShapeType::Rectangle,
        };
    }
};

class PolygonDraft : public Draft {
public:
    explicit PolygonDraft(DraftContext ctx) : Draft(std::move(ctx)) {}

    std::string name() const override { return "Polygon"; }

    void on_mouse_button(int button, int action, int /*mods*/, const Vertex2d& world) override {
        if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS) {
            return;
        }
        add_point(world);
    }

    void on_mouse_move(const Vertex2d& world) override { update_preview(world); }

    void on_key(int key, int action) override {
        if (action != GLFW_PRESS) {
            return;
        }
        if (key == GLFW_KEY_ESCAPE || key == GLFW_KEY_ENTER) {
            if (points_.size() >= 3) {
                auto points = points_;
                reset();
                commit(make_commit(std::move(points)));
            } else {
                reset();
            }
        }
    }

    void refresh_style() override {
        auto style = current_style();
        for (auto& seg : segments_) {
            seg->config.color = style.stroke_color;
            seg->config.stroke = style.stroke_width;
        }
        if (preview_segment_) {
            preview_segment_->config.color = preview_color();
            preview_segment_->config.stroke = style.stroke_width;
        }
    }

    void reset() override {
        points_.clear();
        segments_.clear();
        preview_segment_.reset();
    }

private:
    std::vector<Vertex2d> points_;
    std::vector<std::unique_ptr<LineEntity>> segments_;
    std::unique_ptr<LineEntity> preview_segment_;

    void add_point(const Vertex2d& point) {
        if (!canvas()) {
            return;
        }
        points_.push_back(point);
        if (points_.size() >= 2) {
            add_segment(points_[points_.size() - 2], points_.back());
        }
        preview_segment_.reset();
    }

    void add_segment(const Vertex2d& start, const Vertex2d& end) {
        auto style = current_style();
        Line line{
            .start = start,
            .end = end,
            .color = style.stroke_color,
            .stroke = style.stroke_width,
        };
        auto segment = canvas()->draw(line);
        segment->set_priority(allocate_working_priority());
        segments_.push_back(std::move(segment));
    }

    void update_preview(const Vertex2d& world) {
        if (points_.empty() || !canvas()) {
            preview_segment_.reset();
            return;
        }
        if (!preview_segment_) {
            Line line{
                .start = points_.back(),
                .end = world,
                .color = preview_color(),
                .stroke = current_style().stroke_width,
            };
            preview_segment_ = canvas()->draw(line);
            preview_segment_->set_priority(allocate_working_priority());
        } else {
            preview_segment_->config.start = points_.back();
            preview_segment_->config.end = world;
        }
    }

    DraftCommit make_commit(std::vector<Vertex2d> points) {
        if (!canvas()) {
            return {};
        }
        if (points.size() < 3) {
            return {};
        }
        auto style = current_style();
        Polygon polygon{
            .points = points,
            .color = style.stroke_color,
            .fill_color = style.fill_color,
            .stroke = style.stroke_width,
        };
        auto entity = canvas()->draw(polygon);
        auto captured_points = std::move(points);
        auto rebuild = [captured_points =
                            std::move(captured_points)](Canvas* canvas, const DraftStyle& style) {
            Polygon cfg{
                .points = captured_points,
                .color = style.stroke_color,
                .fill_color = style.fill_color,
                .stroke = style.stroke_width,
            };
            return canvas->draw(cfg);
        };
        return DraftCommit{
            .entity = std::move(entity),
            .rebuild = rebuild,
            .shape_type = ShapeType::Polygon,
        };
    }
};

class PolylineDraft : public Draft {
public:
    explicit PolylineDraft(DraftContext ctx) : Draft(std::move(ctx)) {}

    std::string name() const override { return "Polyline"; }

    void on_mouse_button(int button, int action, int /*mods*/, const Vertex2d& world) override {
        if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS) {
            return;
        }
        add_point(world);
    }

    void on_mouse_move(const Vertex2d& world) override { update_preview(world); }

    void on_key(int key, int action) override {
        if (action != GLFW_PRESS) {
            return;
        }
        if ((key == GLFW_KEY_ENTER || key == GLFW_KEY_ESCAPE) && points_.size() >= 2) {
            auto points = points_;
            reset();
            commit(make_commit(std::move(points)));
        }
        if (key == GLFW_KEY_ESCAPE) {
            reset();
            return;
        }
    }

    void refresh_style() override {
        auto style = current_style();
        for (auto& seg : segments_) {
            seg->config.color = style.stroke_color;
            seg->config.stroke = style.stroke_width;
        }
        if (preview_segment_) {
            preview_segment_->config.color = preview_color();
            preview_segment_->config.stroke = style.stroke_width;
        }
    }

    void reset() override {
        points_.clear();
        segments_.clear();
        preview_segment_.reset();
    }

private:
    std::vector<Vertex2d> points_;
    std::vector<std::unique_ptr<LineEntity>> segments_;
    std::unique_ptr<LineEntity> preview_segment_;

    void add_point(const Vertex2d& point) {
        if (!canvas()) {
            return;
        }
        points_.push_back(point);
        if (points_.size() >= 2) {
            add_segment(points_[points_.size() - 2], points_.back());
        }
        preview_segment_.reset();
    }

    void add_segment(const Vertex2d& start, const Vertex2d& end) {
        auto style = current_style();
        Line line{
            .start = start,
            .end = end,
            .color = style.stroke_color,
            .stroke = style.stroke_width,
        };
        auto segment = canvas()->draw(line);
        segment->set_priority(allocate_working_priority());
        segments_.push_back(std::move(segment));
    }

    void update_preview(const Vertex2d& world) {
        if (points_.empty() || !canvas()) {
            preview_segment_.reset();
            return;
        }
        if (!preview_segment_) {
            Line line{
                .start = points_.back(),
                .end = world,
                .color = preview_color(),
                .stroke = current_style().stroke_width,
            };
            preview_segment_ = canvas()->draw(line);
            preview_segment_->set_priority(allocate_working_priority());
        } else {
            preview_segment_->config.start = points_.back();
            preview_segment_->config.end = world;
        }
    }

    DraftCommit make_commit(std::vector<Vertex2d> points) {
        if (!canvas() || points.size() < 2) {
            return {};
        }
        auto style = current_style();
        Polyline polyline{
            .points = std::move(points),
            .color = style.stroke_color,
            .stroke = style.stroke_width,
        };
        auto captured_points = polyline.points;
        auto entity = canvas()->draw(polyline);
        auto rebuild = [captured_points =
                            std::move(captured_points)](Canvas* canvas, const DraftStyle& style) {
            Polyline cfg{
                .points = captured_points,
                .color = style.stroke_color,
                .stroke = style.stroke_width,
            };
            return canvas->draw(cfg);
        };
        return DraftCommit{
            .entity = std::move(entity),
            .rebuild = rebuild,
            .shape_type = ShapeType::Polyline,
        };
    }
};

class CircleDraft : public Draft {
public:
    explicit CircleDraft(DraftContext ctx) : Draft(std::move(ctx)) {}

    std::string name() const override { return "Circle"; }

    void on_mouse_button(int button, int action, int /*mods*/, const Vertex2d& world) override {
        if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS) {
            return;
        }
        if (!center_) {
            center_ = world;
            return;
        }
        auto radius = distance(*center_, world);
        if (radius <= 0.0) {
            reset();
            return;
        }
        auto center = *center_;
        reset();
        commit(make_commit(center, radius));
    }

    void on_mouse_move(const Vertex2d& world) override {
        if (!center_) {
            return;
        }
        update_preview(world);
    }

    void on_key(int key, int action) override {
        if (action != GLFW_PRESS) {
            return;
        }
        if (key == GLFW_KEY_ESCAPE) {
            reset();
        }
    }

    void refresh_style() override {
        if (preview_circle_) {
            preview_circle_->config.color = preview_color();
            preview_circle_->config.stroke = current_style().stroke_width;
        }
    }

    void reset() override {
        center_.reset();
        preview_circle_.reset();
    }

private:
    std::optional<Vertex2d> center_;
    std::unique_ptr<CircleEntity> preview_circle_;

    static double distance(const Vertex2d& a, const Vertex2d& b) {
        double dx = a.x - b.x;
        double dy = a.y - b.y;
        return std::hypot(dx, dy);
    }

    void update_preview(const Vertex2d& world) {
        if (!canvas() || !center_) {
            return;
        }
        double radius = distance(*center_, world);
        if (radius <= 0.0) {
            preview_circle_.reset();
            return;
        }
        Circle circle{
            .center = *center_,
            .radius = radius,
            .color = preview_color(),
            .fill_color = std::nullopt,
            .stroke = current_style().stroke_width,
        };
        if (!preview_circle_) {
            preview_circle_ = canvas()->draw(circle);
            preview_circle_->set_priority(allocate_working_priority());
        } else {
            preview_circle_->config = circle;
        }
    }

    DraftCommit make_commit(const Vertex2d& center, double radius) {
        if (!canvas()) {
            return {};
        }
        auto style = current_style();
        Circle circle{
            .center = center,
            .radius = radius,
            .color = style.stroke_color,
            .fill_color = style.fill_color,
            .stroke = style.stroke_width,
        };
        auto entity = canvas()->draw(circle);
        auto captured_center = center;
        auto rebuild = [captured_center, radius](Canvas* canvas, const DraftStyle& style) {
            Circle cfg{
                .center = captured_center,
                .radius = radius,
                .color = style.stroke_color,
                .fill_color = style.fill_color,
                .stroke = style.stroke_width,
            };
            return canvas->draw(cfg);
        };
        return DraftCommit{
            .entity = std::move(entity),
            .rebuild = rebuild,
            .shape_type = ShapeType::Circle,
        };
    }
};

}  // namespace painting

}  // namespace opengl
