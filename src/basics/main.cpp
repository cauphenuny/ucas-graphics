#include "opengl.hpp"

#include <GLUT/glut.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    using namespace opengl;
    themes::current_theme = themes::CATPPUCCIN;
    Canvas canvas(
        CanvasParameters{
            .title = "OpenGL 3D View",
            .background = "background",
            .view_point = {
                .eyeZ = 10,
                .upY = 1,
            }});
    auto line = canvas.create(Line{.start = {-0.8, -0.8}, .end = {0.8, 0.8}, .color = "foreground"});
    auto triangle = canvas.create(
        Triangle{
            .p1 = {-0.5, -0.5}, .p2 = {0.5, -0.5}, .p3 = {-0.5, 0.5}, .color = "foreground"});
    canvas.spin();
    return 0;
}
