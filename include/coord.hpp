#pragma once

#include <OpenGL/gl.h>
#include <OpenGL/gltypes.h>
#include <fmt/format.h>
#include <glfw/glfw3.h>
#include <magic_enum/magic_enum.hpp>


namespace opengl {

struct Vertex2d {
    GLdouble x, y;
    Vertex2d(GLdouble x, GLdouble y) : x(x), y(y) {}
    template <typename T> Vertex2d(std::initializer_list<T> init) {
        assert(init.size() == 2);
        x = *init.begin();
        y = *(std::next(init.begin()));
    }
};


}  // namespace opengl

namespace fmt {
template <> struct formatter<opengl::Vertex2d> {
    // parse is required but we don't support any format specifications
    constexpr auto parse(fmt::format_parse_context& ctx) -> decltype(ctx.begin()) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const opengl::Vertex2d& p, FormatContext& ctx) const -> decltype(ctx.out()) {
        return fmt::format_to(ctx.out(), "Vertex2d({}, {})", p.x, p.y);
    }
};

}  // namespace fmt
