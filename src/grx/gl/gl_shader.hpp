#pragma once

#include <vector>
#include <span>

#include <core/moveonly_trivial.hpp>
#include <core/utility/move.hpp>
#include <grx/basic_types.hpp>
#include <grx/gl/gl_shader_basic.hpp>

namespace grx::gl
{
class gl_shader_compile_error : public gl_shader_exception {
public:
    gl_shader_compile_error(std::string message): msg(core::mov(message)) {}

    const char* what() const noexcept override {
        return msg.data();
    }

private:
    std::string msg;
};

template <gl_shader_type Type>
class gl_shader {
public:
    using gl_id_t = core::moveonly_trivial<uint, uint(-1)>;

    gl_shader(const std::string& code) {
        compile(code);
    }

    gl_shader(gl_shader&&) noexcept = default;
    gl_shader& operator=(gl_shader&&) noexcept = default;

    ~gl_shader() {
        glDeleteShader(gl_id);
    }

    uint id() const {
        return gl_id;
    }

private:
    void compile(std::span<const std::byte> code) {
        gl_id = glCreateShader(gl_type(Type));

        //std::array<const char*, 3> data{GL_VERSION_CORE.data(), (const char*)code.data()};
        //std::array<int, 3>         length{int(GL_VERSION_CORE.size()), int(code.size())};
        //GLsizei block_count = 2;

        //if constexpr (Type == gl_shader_type::compute) {
        //    data[2]   = data[1];
        //    data[1]   = GL_COMPUTE_VARIABLE_GROUP.data();
        //    length[2] = length[1];
        //    length[1] = int(GL_COMPUTE_VARIABLE_GROUP.size());
        //    ++block_count;
        //}

        auto data = (const char*)code.data();
        auto len = int(code.size());
        glShaderSource(gl_id, 1, &data, &len);
        glCompileShader(gl_id);

        int rc = GL_FALSE;
        glGetShaderiv(gl_id, GL_COMPILE_STATUS, &rc);

        if (rc == GL_FALSE) {
            int log_length;
            glGetShaderiv(gl_id, GL_INFO_LOG_LENGTH, &log_length);

            std::string msg;
            msg.resize(size_t(log_length));
            glGetShaderInfoLog(gl_id, log_length, nullptr, msg.data());

            glDeleteShader(gl_id);
            gl_id.reset();

            throw gl_shader_compile_error("gl_shader::compile(): " + msg);
        }
    }

    void compile(std::string_view code) {
        compile(std::span{(const std::byte*)code.data(), code.size()});
    }

    gl_id_t gl_id;
};
} // namespace grx::gl
