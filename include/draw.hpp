#pragma once

#include "color.hpp"
#include "coord.hpp"

#include <OpenGL/gl.h>
#include <OpenGL/gltypes.h>
#include <algorithm>
#include <cmath>
#include <fmt/format.h>
#include <glfw/glfw3.h>
#include <limits>
#include <magic_enum/magic_enum.hpp>
#include <numbers>
#include <vector>

namespace opengl::draw {

inline constexpr double kLineWidthScale = 0.03;

inline void rect_filled(const opengl::Vertex2d& center, double w, double h, opengl::RGBColor color);
inline void
circle_filled(const opengl::Vertex2d& center, double radius, opengl::RGBColor color, int segments);

inline void line(Vertex2d start, Vertex2d end, RGBColor color, double width = 1.0) {
    double scaled_width = width * kLineWidthScale;
    if (scaled_width <= 0.0) {
        return;
    }

    double dx = end.x - start.x;
    double dy = end.y - start.y;
    double length = std::hypot(dx, dy);
    if (length <= std::numeric_limits<double>::epsilon()) {
        rect_filled(start, scaled_width, scaled_width, color);
        return;
    }

    Vertex2d center{(start.x + end.x) * 0.5, (start.y + end.y) * 0.5};
    double angle_deg = std::atan2(dy, dx) * 180.0 / std::numbers::pi_v<double>;

    glPushMatrix();
    glTranslated(center.x, center.y, 0.0);
    glRotated(angle_deg, 0.0, 0.0, 1.0);
    rect_filled(Vertex2d{0.0, 0.0}, length, scaled_width, color);
    glPopMatrix();
}

namespace detail {

inline constexpr double kPointEpsilon = 1e-9;

inline void append_point(std::vector<Vertex2d>& points, const Vertex2d& point) {
    if (!points.empty()) {
        const auto& last = points.back();
        if (std::abs(last.x - point.x) <= kPointEpsilon &&
            std::abs(last.y - point.y) <= kPointEpsilon) {
            return;
        }
    }
    points.emplace_back(point.x, point.y);
}

inline void
draw_polyline(const std::vector<Vertex2d>& points, bool closed, RGBColor color, double width) {
    if (points.size() < 2) {
        return;
    }

    double joint_radius = 0.5 * width * kLineWidthScale;
    if (joint_radius > 0.0) {
        constexpr int kJointSegments = 18;
        for (const auto& p : points) {
            circle_filled(p, joint_radius, color, kJointSegments);
        }
    }

    for (std::size_t i = 1; i < points.size(); ++i) {
        line(points[i - 1], points[i], color, width);
    }
    if (closed) {
        line(points.back(), points.front(), color, width);
    }
}

}  // namespace detail

inline void triangle(Vertex2d p1, Vertex2d p2, Vertex2d p3, RGBColor color) {
    // spdlog::info("Drawing triangle with vertices: {}, {}, {}", p1, p2, p3);
    glColor3d(color.red, color.green, color.blue);
    glBegin(GL_TRIANGLES);
    for (auto p : {p1, p2, p3}) {
        glVertex2d(p.x, p.y);
    }
    glEnd();
}

// 画圆（描边）
// center: 圆心, radius: 半径, color: 颜色, segments: 分段数（越大越圆）
inline void circle_outline(
    const opengl::Vertex2d& center, double radius, opengl::RGBColor color, int segments = 64,
    double line_stroke = 1.0) {
    if (radius <= 0.0) {
        return;
    }
    int segs = std::max(3, segments);
    std::vector<Vertex2d> points;
    points.reserve(segs);
    for (int i = 0; i < segs; ++i) {
        double angle = 2.0 * std::numbers::pi_v<double> * static_cast<double>(i) / segs;
        detail::append_point(
            points,
            Vertex2d{center.x + radius * std::cos(angle), center.y + radius * std::sin(angle)});
    }
    detail::draw_polyline(points, true, color, line_stroke);
}

// 画填充圆（triangle fan）
inline void circle_filled(
    const opengl::Vertex2d& center, double radius, opengl::RGBColor color, int segments = 64) {
    glColor3d(color.red, color.green, color.blue);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2d(center.x, center.y);  // 中心点
    for (int i = 0; i <= segments; ++i) {
        double a = 2.0 * M_PI * i / segments;
        double x = center.x + radius * cos(a);
        double y = center.y + radius * sin(a);
        glVertex2d(x, y);
    }
    glEnd();
}

// 画弧（从 start_deg 开始，扫过 sweep_deg 度）
// start_deg, sweep_deg 以度为单位，positive 是逆时针 (标准数学方向)
inline void arc_outline(
    const opengl::Vertex2d& center, double radius, double start_deg, double sweep_deg,
    opengl::RGBColor color, int segments = 64, double line_stroke = 1.0) {
    if (radius <= 0.0 || sweep_deg == 0.0) {
        return;
    }
    int base_segments = std::max(3, segments);
    int segs = std::max(
        1, static_cast<int>(
               std::ceil(std::abs(sweep_deg) / 360.0 * static_cast<double>(base_segments))));
    std::vector<Vertex2d> points;
    points.reserve(segs + 1);
    for (int i = 0; i <= segs; ++i) {
        double t = static_cast<double>(i) / segs;
        double ang = (start_deg + t * sweep_deg) * std::numbers::pi_v<double> / 180.0;
        detail::append_point(
            points, Vertex2d{center.x + radius * std::cos(ang), center.y + radius * std::sin(ang)});
    }
    detail::draw_polyline(points, false, color, line_stroke);
}

inline void
rect_filled(const opengl::Vertex2d& center, double w, double h, opengl::RGBColor color) {
    glColor3d(color.red, color.green, color.blue);
    glBegin(GL_QUADS);
    double half_w = w * 0.5;
    double half_h = h * 0.5;
    glVertex2d(center.x - half_w, center.y - half_h);
    glVertex2d(center.x + half_w, center.y - half_h);
    glVertex2d(center.x + half_w, center.y + half_h);
    glVertex2d(center.x - half_w, center.y + half_h);
    glEnd();
}

inline void rect_outline(
    const opengl::Vertex2d& center, double w, double h, opengl::RGBColor color,
    double line_stroke = 1.0) {
    double half_w = w * 0.5;
    double half_h = h * 0.5;
    std::vector<Vertex2d> points;
    points.reserve(4);
    points.emplace_back(center.x - half_w, center.y - half_h);
    points.emplace_back(center.x + half_w, center.y - half_h);
    points.emplace_back(center.x + half_w, center.y + half_h);
    points.emplace_back(center.x - half_w, center.y + half_h);
    detail::draw_polyline(points, true, color, line_stroke);
}

// 画带圆角的矩形（填充）
// x,y 为矩形中心，w,h 为宽高，r 为圆角半径（不要超过 min(w,h)/2）
inline void rounded_rect_filled(
    const opengl::Vertex2d& center, double w, double h, double r, opengl::RGBColor color,
    int corner_segments = 16) {
    // 保证半径不超限
    r = std::max(0.0, std::min(r, std::min(w, h) * 0.5));
    double half_w = w * 0.5;
    double half_h = h * 0.5;
    // 四个角中心
    opengl::Vertex2d c1{center.x + half_w - r, center.y + half_h - r};  // top-right
    opengl::Vertex2d c2{center.x - half_w + r, center.y + half_h - r};  // top-left
    opengl::Vertex2d c3{center.x - half_w + r, center.y - half_h + r};  // bottom-left
    opengl::Vertex2d c4{center.x + half_w - r, center.y - half_h + r};  // bottom-right

    glColor3d(color.red, color.green, color.blue);

    // Strategy: draw center rect + four edge rects + four quarter-circles (triangle fan each)
    // 1) center rectangle
    glBegin(GL_QUADS);
    glVertex2d(center.x - half_w + r, center.y - half_h + r);
    glVertex2d(center.x + half_w - r, center.y - half_h + r);
    glVertex2d(center.x + half_w - r, center.y + half_h - r);
    glVertex2d(center.x - half_w + r, center.y + half_h - r);
    glEnd();

    // 2) edge rectangles (left, right, top, bottom)
    glBegin(GL_QUADS);
    // left
    glVertex2d(center.x - half_w, center.y - half_h + r);
    glVertex2d(center.x - half_w + r, center.y - half_h + r);
    glVertex2d(center.x - half_w + r, center.y + half_h - r);
    glVertex2d(center.x - half_w, center.y + half_h - r);
    // right
    glVertex2d(center.x + half_w - r, center.y - half_h + r);
    glVertex2d(center.x + half_w, center.y - half_h + r);
    glVertex2d(center.x + half_w, center.y + half_h - r);
    glVertex2d(center.x + half_w - r, center.y + half_h - r);
    // bottom
    glVertex2d(center.x - half_w + r, center.y - half_h);
    glVertex2d(center.x + half_w - r, center.y - half_h);
    glVertex2d(center.x + half_w - r, center.y - half_h + r);
    glVertex2d(center.x - half_w + r, center.y - half_h + r);
    // top
    glVertex2d(center.x - half_w + r, center.y + half_h - r);
    glVertex2d(center.x + half_w - r, center.y + half_h - r);
    glVertex2d(center.x + half_w - r, center.y + half_h);
    glVertex2d(center.x - half_w + r, center.y + half_h);
    glEnd();

    // 3) 四个角，画四个四分之一扇形（三角扇）
    auto quarter_fan = [&](const opengl::Vertex2d& cc, double start_deg) {
        glBegin(GL_TRIANGLE_FAN);
        glVertex2d(cc.x, cc.y);
        int segs = std::max(4, corner_segments);
        for (int i = 0; i <= segs; ++i) {
            double ang = (start_deg + (90.0 * i / segs)) * M_PI / 180.0;
            double x = cc.x + r * cos(ang);
            double y = cc.y + r * sin(ang);
            glVertex2d(x, y);
        }
        glEnd();
    };

    // top-right: start at 0 deg (to the right), sweep +90 -> up
    quarter_fan(c1, 0.0);
    // top-left: start at 90 -> left/up
    quarter_fan(c2, 90.0);
    // bottom-left: 180 -> left/down
    quarter_fan(c3, 180.0);
    // bottom-right: 270 -> right/down
    quarter_fan(c4, 270.0);
}

// 圆角矩形描边（只画外圈）
// 描边用 GL_LINE_STRIP 组合四段弧和四段直线
inline void rounded_rect_outline(
    const opengl::Vertex2d& center, double w, double h, double r, opengl::RGBColor color,
    int corner_segments = 16, double line_stroke = 1.0) {
    r = std::max(0.0, std::min(r, std::min(w, h) * 0.5));
    double half_w = w * 0.5, half_h = h * 0.5;
    opengl::Vertex2d c1{center.x + half_w - r, center.y + half_h - r};  // top-right
    opengl::Vertex2d c2{center.x - half_w + r, center.y + half_h - r};  // top-left
    opengl::Vertex2d c3{center.x - half_w + r, center.y - half_h + r};  // bottom-left
    opengl::Vertex2d c4{center.x + half_w - r, center.y - half_h + r};  // bottom-right

    int segs = std::max(4, corner_segments);
    std::vector<Vertex2d> points;
    points.reserve(static_cast<std::size_t>((segs + 1) * 4 + 8));

    auto append_arc = [&](const Vertex2d& arc_center, double start_deg) {
        for (int i = 0; i <= segs; ++i) {
            double ang = (start_deg + (90.0 * i / segs)) * std::numbers::pi_v<double> / 180.0;
            detail::append_point(
                points,
                Vertex2d{arc_center.x + r * std::cos(ang), arc_center.y + r * std::sin(ang)});
        }
    };

    append_arc(c2, 90.0);  // top-left arc (90 -> 180)
    detail::append_point(points, Vertex2d{center.x - half_w, center.y + half_h - r});
    detail::append_point(points, Vertex2d{center.x - half_w, center.y - half_h + r});

    append_arc(c3, 180.0);  // bottom-left arc (180 -> 270)
    detail::append_point(points, Vertex2d{center.x - half_w + r, center.y - half_h});
    detail::append_point(points, Vertex2d{center.x + half_w - r, center.y - half_h});

    append_arc(c4, 270.0);  // bottom-right arc (270 -> 360)
    detail::append_point(points, Vertex2d{center.x + half_w, center.y - half_h + r});
    detail::append_point(points, Vertex2d{center.x + half_w, center.y + half_h - r});

    append_arc(c1, 0.0);  // top-right arc (0 -> 90)

    detail::draw_polyline(points, true, color, line_stroke);
}

}  // namespace opengl::draw
