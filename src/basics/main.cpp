#include "canvas.hpp"
#include "color.hpp"
#include "entity.hpp"

#include <GLUT/glut.h>
#include <stdio.h>
#include <stdlib.h>

using namespace opengl;

struct Computer {
    std::set<std::unique_ptr<Entity>> parts;
    Computer(Canvas* canvas, double center_x = 0.0, double center_y = 0.0) {
        auto screen_x = center_x, screen_y = center_y + 0.5;
        double screen_w = 3, screen_h = 2;
        double base_delta = 0.4;
        double base_x = screen_x, base_y = screen_y - screen_h / 2 - base_delta;
        double base_w = 1.5, base_h = 0.5;
        double margin = 0.2;
        auto base_color = mix("foreground", "background", 0.5);
        auto margin_color = mix("foreground", "background", 0.8);
        parts.insert(canvas->draw(
            Rectangle{
                .center = {base_x, base_y},
                .width = base_w,
                .height = base_h,
                .corner_radius = 0.15,
                .fill_color = base_color,
            }));
        parts.insert(canvas->draw(
            Rectangle{
                .center = {base_x, screen_y - screen_h / 2},
                .width = base_w * 0.6,
                .height = base_delta * 2,
                .fill_color = base_color,
            }));
        parts.insert(canvas->draw(
            Rectangle{
                .center = {screen_x, screen_y},
                .width = screen_w,
                .height = screen_h,
                .fill_color = margin_color,
            }));
        parts.insert(canvas->draw(
            Rectangle{
                .center = {screen_x, screen_y},
                .width = screen_w - margin,
                .height = screen_h - margin,
                .corner_radius = 0.2,
                .fill_color = "bright_blue",
            }));
        double triangle_l1 = 0.6, triangle_l2 = 0.4;
        parts.insert(canvas->draw(
            Triangle{
                .p1 = {screen_x - triangle_l1, screen_y - triangle_l2},
                .p2 = {screen_x + triangle_l1, screen_y - triangle_l2},
                .p3 = {screen_x, screen_y + triangle_l2 * 1.5},
                .fill_color = "bright_yellow",
            }));
        double circle_r = 0.2;
        parts.insert(canvas->draw(
            Circle{
                .center = {screen_x, screen_y},
                .radius = circle_r,
                .fill_color = "bright_red",
            }));
    }
};

void computer_demo(Canvas& canvas) {
    Computer computer(&canvas, 0.0, 0.0);
    canvas.spin();
}

void interact_demo(Canvas& canvas) {
    canvas.spin();
    return;
}

int main(int argc, char** argv) {
    themes::current_theme = themes::CATPPUCCIN;
    Canvas canvas(
        CanvasParameters{
            .title = "Project 1",
            .background = "background",
            .view_point = {
                .eyeZ = 10,
                .upY = 1,
            }});
    computer_demo(canvas);
    interact_demo(canvas);
    return 0;
}
