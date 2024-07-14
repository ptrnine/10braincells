#pragma once

#include <GL/glew.h>

#include <core/exception.hpp>
#include <grx/using_std.hpp>

namespace grx::gl
{

//constexpr inline auto GL_VERSION_CORE           = "#version 430 core\n"sv;
//constexpr inline auto GL_COMPUTE_VARIABLE_GROUP = "#extension GL_ARB_compute_variable_group_size: enable\n"sv;

enum class gl_shader_type { vertex = 0, fragment, geometry, compute };

constexpr GLenum gl_type(gl_shader_type type) {
    switch (type) {
    case gl_shader_type::fragment: return GL_FRAGMENT_SHADER;
    case gl_shader_type::vertex: return GL_VERTEX_SHADER;
    case gl_shader_type::geometry: return GL_GEOMETRY_SHADER;
    case gl_shader_type::compute: return GL_COMPUTE_SHADER;
    }
}

class gl_shader_exception : public core::exception {};
} // namespace grx::gl
