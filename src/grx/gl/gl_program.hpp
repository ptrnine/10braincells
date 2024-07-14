#pragma once

#include <grx/gl/gl_shader.hpp>
#include <grx/gl/gl_uniform.hpp>
#include <grx/gl/gl_ctx.hpp>
#include <util/log.hpp>

namespace grx::gl
{
class gl_program_link_error : public gl_shader_exception {
public:
    gl_program_link_error(std::string message): msg(core::mov(message)) {}

    const char* what() const noexcept override {
        return msg.data();
    }

private:
    std::string msg;
};

class gl_program {
public:
    using gl_id_t = core::moveonly_trivial<uint, uint(-1)>;

    template <gl_shader_type... Types>
    gl_program(const gl_shader<Types>&... shaders): gl_id(glCreateProgram()) {
        util::glog().debug("[gl_program]: create program {}", gl_id);
        (attach_shader(shaders), ...);
        if (sizeof...(Types))
            link();
    }

    gl_program(gl_program&&) noexcept = default;
    gl_program& operator=(gl_program&&) noexcept = default;

    ~gl_program() {
        destroy();
    }

    uint id() const {
        return gl_id;
    }

    void use() const {
        ctx().use_program(gl_id);
    }

    template <gl_uniformable T>
    gl_uniform<T> get_uniform(const std::string& name) const {
        return gl_uniform<T>(gl_id, name);
    }

private:
    void destroy() {
        if (gl_id.not_default()) {
            util::glog().debug("[gl_program]: delete {}", gl_id);
            glDeleteProgram(gl_id);
        }
    }

    template <gl_shader_type Type>
    void attach_shader(const gl_shader<Type>& shader) {
        glAttachShader(gl_id, shader.id());
        util::glog().debug("[gl_program]: attach shader: {}", shader.id());
    }

    void link() {
        glLinkProgram(gl_id);
        util::glog().debug("[gl_program]: link program {}", gl_id);

        int rc = GL_FALSE;
        glGetProgramiv(gl_id, GL_LINK_STATUS, &rc);

        if (rc == GL_FALSE) {
            int log_length;
            glGetProgramiv(gl_id, GL_INFO_LOG_LENGTH, &log_length);

            std::string msg;
            msg.resize(size_t(log_length));
            glGetProgramInfoLog(gl_id, log_length, nullptr, msg.data());

            glDeleteProgram(gl_id);

            throw gl_program_link_error("gl_program::link(): " + msg);
        }
    }

private:
    gl_id_t gl_id;
};
} // namespace grx::gl
