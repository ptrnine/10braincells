#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <grx/camera.hpp>
#include <grx/gl/gl_program.hpp>
#include <grx/gl/gl_postprocess_mgr.hpp>
#include <grx/gl/gl_ctx.hpp>

namespace grx::gl
{
//template <gl_render_target_settings RenderTargetSettings>
class gl_window {
public:
    static inline constexpr auto RenderTargetSettings = gl_render_target_settings{gl_color_fmt::rgb16f, gl_filtering::linear, 0};
    using render_target_t = gl_postprocess_mgr<RenderTargetSettings>;

    static inline constexpr int    quad_texture_binding = 0;
    static inline constexpr size_t max_cameras          = 32;

    gl_window(vec2u size): _wnd(init_wnd(size)), _render_target(size) {

        gl_shader<gl_shader_type::vertex> vs{
            "#version 460 core\n"
            "in vec3 pos_ms;"
            "out vec2 uv;"
            "void main() {"
            "    gl_Position = vec4(pos_ms, 1.0);"
            "    uv = (pos_ms.xy + vec2(1.0, 1.0)) * 0.5;"
            "}",
        };
        gl_shader<gl_shader_type::fragment> fs{
            "#version 460 core\n"
            "uniform sampler2D screen;"
            "in vec2 uv;"
            "out vec4 color;"
            "void main() {"
            "    color = texture(screen, uv);"
            "}",
        };
        _passthrough = gl_program{vs, fs};
        _passthrough.get_uniform<int>("screen") = quad_texture_binding;

        if (!ctx().get_main_window<gl_window>())
            ctx().set_main_window(this);

        ctx().bind_previous_ctx();
    }

    ~gl_window() {
        glfwDestroyWindow(_wnd);
    }

    void set_title(const std::string& value) {
        glfwSetWindowTitle(_wnd, value.data());
    }

    std::string_view get_title() const {
        return glfwGetWindowTitle(_wnd);
    }

    bool attach_camera(camera& camera) {
        return _cameras.emplace(&camera)[1_c];
    }

    bool detach_camera(camera& camera) {
        return _cameras.erase(&camera);
    }

    void enable_mouse_pointer(bool value = true) {
        glfwSetInputMode(_wnd, GLFW_CURSOR, value ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
    }

    bool is_mouse_pointer_enabled() const {
        return glfwGetInputMode(_wnd, GLFW_CURSOR) == GLFW_CURSOR_NORMAL;
    }

    GLFWwindow* get_impl() const {
        return _wnd;
    }

protected:
    static GLFWwindow* init_wnd(vec2u size) {
        auto main_window = ctx().get_main_window<gl_window>();
        auto wnd = glfwCreateWindow(int(size.x()), int(size.y()), "", nullptr, main_window ? main_window->_wnd : nullptr);
        R_ASSERT(wnd, "glfwCreateWindow() failed");

        ctx().bind_ctx(wnd);
        if (!ctx().get_main_window<gl_window>()) {
            glewExperimental = true;
            auto rc = glewInit();
            R_ASSERT(rc == GLEW_OK, "glewInit() failed");
            ctx().setup_debug_callback();
        }

        // Input
        glfwSetInputMode(wnd, GLFW_STICKY_KEYS, GL_TRUE);
        //glfwSetInputMode(wnd, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

        // Render params
        //glCullFace(GL_BACK);
        //glEnable(GL_CULL_FACE);
        //glEnable(GL_DEPTH_TEST);
        //glDepthFunc(GL_LESS);

        return wnd;
    }

private:
    GLFWwindow*     _wnd;
    render_target_t _render_target;
    gl_program      _passthrough;

    core::static_ptr_set<camera*, max_cameras> _cameras;
};
} // namespace grx::gl
