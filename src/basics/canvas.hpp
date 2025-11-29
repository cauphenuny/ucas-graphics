#pragma once

#include "color.hpp"
#include "entity.hpp"

#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <algorithm>
#include <cassert>
#include <cctype>
#include <fmt/format.h>
#include <glfw/glfw3.h>
#include <magic_enum/magic_enum.hpp>
#include <map>
#include <memory>
#include <set>
#include <spdlog/spdlog.h>
#include <string>
#include <sys/types.h>
#include <vector>

namespace opengl {

struct CanvasParameters {
    std::string title;
    struct {
        int width{800}, height{800};
    } display_size;
    Color background{"background"};
    struct {
        GLdouble left{-5}, right{5}, bottom{-5}, top{5};
        GLdouble zNear{5}, zFar{15};
    } projection;
    struct {
        GLdouble eyeX, eyeY, eyeZ;
        GLdouble centerX, centerY, centerZ;
        GLdouble upX, upY, upZ;
    } view_point;
};

struct EntityAttribute {
    int priority;
};

inline struct GlfwContext {
    GlfwContext() {
        if (!glfwInit()) {
            throw std::runtime_error("glfw init failed");
        }
    }
    ~GlfwContext() { glfwTerminate(); }
} glfw_context;

struct Canvas;

struct ActionHandler {
    virtual void on_key(int key, int action) = 0;
    virtual void on_mouse_button(int button, int action, int mods) = 0;
    virtual void on_mouse_move(double xpos, double ypos) = 0;
    virtual ~ActionHandler() = default;
    Canvas* canvas{nullptr};
    void attach(Canvas* canvas) { this->canvas = canvas; }
};

struct Canvas {
    const CanvasParameters params;
    GLFWwindow* window;
    std::set<Entity*> entities;
    std::map<Entity*, EntityAttribute> entity_attributes;
    int priority_counter{0};
    std::unique_ptr<ActionHandler> action_handler;

    Canvas(const CanvasParameters& params = CanvasParameters()) : params(params) {}

    void window_init() {
        auto [w, h] = this->params.display_size;
        spdlog::info("creating GLFW window width: {}, height: {}", w, h);
        this->window = glfwCreateWindow(w, h, this->params.title.c_str(), NULL, NULL);
        if (!window) {
            throw std::runtime_error("glfw create window failed");
        }
    }

    void init() {
        auto& [title, display_size, bg, proj, view] = this->params;
        glfwMakeContextCurrent(this->window);
        glClearColor(bg.red, bg.green, bg.blue, bg.alpha);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(proj.left, proj.right, proj.bottom, proj.top, proj.zNear, proj.zFar);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        gluLookAt(
            view.eyeX, view.eyeY, view.eyeZ, view.centerX, view.centerY, view.centerZ, view.upX,
            view.upY, view.upZ);
    }

    void attach_handler(ActionHandler* handler) {
        spdlog::info("attaching action handler to canvas {}", (void*)this);
        auto key_callback = [](GLFWwindow* window, int key, int scancode, int action, int mods) {
            ActionHandler* self = static_cast<ActionHandler*>(glfwGetWindowUserPointer(window));
            self->on_key(key, action);
        };
        auto mouse_button_callback = [](GLFWwindow* window, int button, int action, int mods) {
            ActionHandler* self = static_cast<ActionHandler*>(glfwGetWindowUserPointer(window));
            self->on_mouse_button(button, action, mods);
        };
        auto mouse_move_callback = [](GLFWwindow* window, double xpos, double ypos) {
            ActionHandler* self = static_cast<ActionHandler*>(glfwGetWindowUserPointer(window));
            self->on_mouse_move(xpos, ypos);
        };
        glfwSetWindowUserPointer(this->window, handler);
        glfwSetKeyCallback(this->window, key_callback);
        glfwSetMouseButtonCallback(this->window, mouse_button_callback);
        glfwSetCursorPosCallback(this->window, mouse_move_callback);
        handler->attach(this);
        spdlog::info("action handler attached");
    }

    void detach_handler() {
        spdlog::info("detaching action handler from canvas {}", (void*)this);
        glfwSetWindowUserPointer(this->window, nullptr);
        glfwSetKeyCallback(this->window, nullptr);
        glfwSetMouseButtonCallback(this->window, nullptr);
        glfwSetCursorPosCallback(this->window, nullptr);
        spdlog::info("action handler detached");
        this->action_handler.reset();
    }

    void spin() {
        spdlog::info("entering main loop");
        spdlog::info("initializing window");
        this->window_init();
        this->init();
        spdlog::info("window initialized");
        if (this->action_handler) {
            spdlog::info("attaching action handler");
            this->attach_handler(this->action_handler.get());
        }
        spdlog::info("start!");
        while (!glfwWindowShouldClose(this->window)) {
            glClear(GL_COLOR_BUFFER_BIT);
            std::vector<Entity*> sorted_entities(entities.begin(), entities.end());
            std::sort(sorted_entities.begin(), sorted_entities.end(), [this](Entity* a, Entity* b) {
                return this->entity_attributes[a].priority < this->entity_attributes[b].priority;
            });
            for (auto entity : sorted_entities) {
                entity->draw();
            }
            glfwSwapBuffers(window);
            glfwPollEvents();
        }
        spdlog::info("exiting main loop");
        glfwDestroyWindow(this->window);
        this->window = nullptr;
        spdlog::info("window destroyed");
    }

    auto get_entity_attr(Entity* entity) -> EntityAttribute {
        return this->entity_attributes.at(entity);
    }
    void set_entity_priority(Entity* entity, int priority) {
        this->entity_attributes.at(entity).priority = priority;
    }
    void add_entity(Entity* entity);
    void delete_entity(Entity* entity);

    auto draw(auto config) {
        auto entity = std::make_unique<typename decltype(config)::EntityType>(this, config);
        spdlog::info("draw: {}, id={}, canvas={}", entity->repr(), (void*)entity.get(), (void*)this);
        return entity;
    }

    void set_action_handler(std::unique_ptr<ActionHandler> handler) {
        this->action_handler = std::move(handler);
    }

    ~Canvas() {
        action_handler.reset();
        for (auto entity : entities) {
            spdlog::warn("canvas destructing with remaining entity {}", (void*)entity);
            entity->container = nullptr;
        }
        spdlog::info("canvas {} destructed", (void*)this);
    }
};

inline Entity::Entity(Canvas* canvas) : container(canvas) { this->container->add_entity(this); }
inline Entity::~Entity() {
    if (this->container) this->container->delete_entity(this);
    spdlog::info("entity {} destructed", (void*)this);
}
inline auto Entity::attribute() const -> EntityAttribute {
    return this->container->get_entity_attr(const_cast<Entity*>(this));
}
inline void Entity::set_priority(int new_priority) const {
    this->container->set_entity_priority(const_cast<Entity*>(this), new_priority);
}

inline void Canvas::add_entity(Entity* entity) {
    entities.insert(entity);
    entity_attributes[entity] = EntityAttribute{priority_counter += 10};
}
inline void Canvas::delete_entity(Entity* entity) {
    auto it = entities.find(entity);
    if (it != entities.end()) {
        entities.erase(it);
    } else {
        throw std::runtime_error(fmt::format("Entity {} not found", (void*)entity));
    }
    spdlog::info("detached entity {} from canvas {}", (void*)entity, (void*)this);
}

}  // namespace opengl

namespace fmt {

template <> struct formatter<opengl::Entity*> {
    constexpr auto parse(fmt::format_parse_context& ctx) -> decltype(ctx.begin()) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const opengl::Entity* c, FormatContext& ctx) const -> decltype(ctx.out()) {
        return fmt::format_to(ctx.out(), "{}", c->repr());
    }
};

}  // namespace fmt