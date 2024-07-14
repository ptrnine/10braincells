#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <core/traits/is_same.hpp>

#include <grx/basic_types.hpp>
#include <grx/using_std.hpp>
#include <util/assert.hpp>
#include <util/log.hpp>

namespace grx::gl {

static_assert(core::is_same<float, GLfloat>);
static_assert(core::is_same<core::uint, GLuint>);
static_assert(core::is_same<char, GLchar>);
static_assert(core::is_same<void, GLvoid>);
static_assert(core::is_same<int, GLsizei>);

static void glfw_error(int id, const char *description) {
    util::glog().error("GLFW error: {}", description);
}

static util::log_level gl_severity_to_log_level(GLenum severity) {
    switch (severity) {
    case GL_DEBUG_SEVERITY_NOTIFICATION:
        return util::log_level::detail;
    case GL_DEBUG_SEVERITY_LOW:
        return util::log_level::info;
    case GL_DEBUG_SEVERITY_MEDIUM:
        return util::log_level::warn;
    case GL_DEBUG_SEVERITY_HIGH:
        return util::log_level::error;
    default:
        return util::log_level::warn;
    }
}

static std::string_view gl_severity_str(GLenum severity) {
    switch (severity) {
    case GL_DEBUG_SEVERITY_NOTIFICATION:
        return "notification"sv;
    case GL_DEBUG_SEVERITY_LOW:
        return "low"sv;
    case GL_DEBUG_SEVERITY_MEDIUM:
        return "medium"sv;
    case GL_DEBUG_SEVERITY_HIGH:
        return "high"sv;
    default:
        return "*unknown*"sv;
    }
}

static std::string_view gl_debug_type_str(GLenum type) {
    switch (type) {
    case GL_DEBUG_TYPE_ERROR:
        return "error"sv;
    case GL_DEBUG_TYPE_PERFORMANCE:
        return "performance"sv;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        return "deprecated behavior"sv;
    case GL_DEBUG_TYPE_MARKER:
        return "marker"sv;
    case GL_DEBUG_TYPE_POP_GROUP:
        return "pop group"sv;
    case GL_DEBUG_TYPE_PORTABILITY:
        return "portability"sv;
    case GL_DEBUG_TYPE_PUSH_GROUP:
        return "push group"sv;
    case GL_DEBUG_TYPE_OTHER:
        return "other"sv;
    default:
        return "*undefined*"sv;
    }
}

static std::string_view gl_debug_source_str(GLenum source) {
    switch (source) {
    case GL_DEBUG_SOURCE_API:
        return "OpenGL API"sv;
    case GL_DEBUG_SOURCE_APPLICATION:
        return "application"sv;
    case GL_DEBUG_SOURCE_SHADER_COMPILER:
        return "GLSL compiler"sv;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
        return "window system"sv;
    case GL_DEBUG_SOURCE_THIRD_PARTY:
        return "thirt party library or debuger"sv;
    case GL_DEBUG_SOURCE_OTHER:
        return "other"sv;
    default:
        return "*undefined*"sv;
    }
}

void GLAPIENTRY gl_message_callback(
        GLenum        source,
        GLenum        type,
        GLuint      /*id*/,
        GLenum        severity,
        GLsizei     /*length*/,
        const GLchar* message,
        const GLvoid* /*user_param*/
) {
    auto level = gl_severity_to_log_level(severity);

    util::glog().log(level, "[{}] {}: {}", gl_debug_type_str(type), gl_debug_source_str(source), std::string_view(message));
}

class gl_ctx {
public:
    static gl_ctx& instance() {
        static gl_ctx inst;
        return inst;
    }

    gl_ctx() {
        glfwSetErrorCallback(&glfw_error);
        R_ASSERT(glfwInit());
        util::glog().debug("glfwInit() ok");

        // Antialiasing
        //glfwWindowHint(GLFW_SAMPLES, 4);
        //glDisable(GL_MULTISAMPLE);

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    }

    ~gl_ctx() {
        glfwTerminate();
    }

    void setup_debug_callback() {
        glEnable(GL_DEBUG_OUTPUT);
        glDebugMessageCallback(gl_message_callback, nullptr);
    }

    void bind_default_framebuffer() {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void bind_vao(uint vao_id) {
        if (binded_vaos.empty() || binded_vaos.back() != vao_id) {
            glBindVertexArray(vao_id);
            binded_vaos.push(vao_id);
        }
    }

    bool bind_previous_vao() {
        binded_vaos.pop();

        if (binded_vaos.empty()) {
            util::glog().warn("There is no previous VAO");
            return false;
        }

        glBindVertexArray(binded_vaos.back());
        return true;
    }

    auto vao_scope(uint vao_id) {
        struct releaser {
            releaser() = default;
            releaser(const releaser&) = delete;
            releaser& operator=(const releaser&) = delete;
            releaser(releaser&&) = delete;
            releaser& operator=(releaser&&) = delete;
            ~releaser() {
                gl_ctx::instance().bind_previous_vao();
            }
        };
        bind_vao(vao_id);
        return releaser{};
    }

    void set_wireframe_enabled(bool value) {
        if (value == wireframe_enabled)
            return;

        if (value)
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        wireframe_enabled = value;
    }

    void set_depth_test_enabled(bool value) {
        if (value == depth_test_enabled)
            return;

        if (value)
            glEnable(GL_DEPTH_TEST);
        else
            glDisable(GL_DEPTH_TEST);

        depth_test_enabled = value;
    }

    void set_depth_mask_enabled(bool value) {
        if (value == depth_test_enabled)
            return;

        if (value)
            glDepthMask(GL_TRUE);
        else
            glDepthMask(GL_FALSE);

        depth_mask_enabled = value;
    }

    void set_cull_face_enabled(bool value) {
        if (value == cull_face_enabled)
            return;

        if (value)
            glEnable(GL_CULL_FACE);
        else
            glDisable(GL_CULL_FACE);

        cull_face_enabled = value;
    }

    void use_program(uint program) {
        if (program != active_program) {
            glUseProgram(program);
            active_program = program;
        }
    }

    uint get_active_program() const {
        return active_program;
    }

    template <typename T>
    T* get_main_window() const {
        return (T*)main_window;
    }

    void set_main_window(auto* window) {
        main_window = window;
    }

    void bind_ctx(GLFWwindow* ctx) {
        binded_ctxs.push(ctx);
        glfwMakeContextCurrent(ctx);
    }

    GLFWwindow* current_ctx() const {
        return binded_ctxs.empty() ? nullptr : binded_ctxs.back();
    }

    void bind_previous_ctx() {
        binded_ctxs.pop();
        glfwMakeContextCurrent(binded_ctxs.empty() ? nullptr : binded_ctxs.back());
    }

    auto ctx_scope(GLFWwindow* wnd) {
        struct scope {
            scope(GLFWwindow* wnd) {
                gl_ctx::instance().bind_ctx(wnd);
            }
            ~scope() {
                gl_ctx::instance().bind_previous_ctx();
            }
            scope(const scope&) = delete;
            scope& operator=(const scope&) = delete;
            scope(scope&&) = delete;
            scope& operator=(scope&&) = delete;
        };
        return scope{wnd};
    }

private:
    /* TODO: replace for static sized vector */
    util::ring_buffer<uint>        binded_vaos = {32};
    util::ring_buffer<GLFWwindow*> binded_ctxs = {32};

    bool  wireframe_enabled  = false;
    bool  depth_test_enabled = false;
    bool  depth_mask_enabled = false;
    bool  cull_face_enabled  = false;
    uint  active_program     = uint(-1);
    void* main_window        = nullptr;
};

inline gl_ctx& ctx() {
    return gl_ctx::instance();
}
} // namespace grx::gl
