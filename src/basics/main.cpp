#include "color.hpp"
#include "canvas.hpp"

#include <GLUT/glut.h>
#include <stdio.h>
#include <stdlib.h>

using namespace opengl;

struct Computer {
    std::set<std::unique_ptr<Entity>> parts;
    Computer(Canvas* canvas, double center_x = 0.0, double center_y = 0.0) {
        auto screen_x = center_x, screen_y = center_y + 0.5;
        auto screen_w = 1.5, screen_h = 1.0;
        auto margin = 0.1;
        parts.insert(canvas->draw(
            Rectangle{
                .center = {screen_x, screen_y},
                .width = screen_w,
                .height = screen_h,
                .is_round = false,
                .color = mix("foreground", "background", 1),
                .filled = true,
            }));
        parts.insert(canvas->draw(
            Rectangle{
                .center = {screen_x, screen_y},
                .width = screen_w - margin,
                .height = screen_h - margin,
                .is_round = true,
                .corner_radius = 0.1,
                .color = "foreground",
                .filled = false,
            }));
    }
};

int main(int argc, char** argv) {
    themes::current_theme = themes::CATPPUCCIN;
    Canvas canvas(
        CanvasParameters{
            .title = "OpenGL 3D View",
            .background = "background",
            .view_point = {
                .eyeZ = 10,
                .upY = 1,
            }});
    Computer computer(&canvas, 0.0, 0.0);
    canvas.spin();
    return 0;
}
