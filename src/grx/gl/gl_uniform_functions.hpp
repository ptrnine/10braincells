#pragma once

#include <span>

#include <GL/glew.h>
#include <glm/matrix.hpp>

#include <core/concepts/number.hpp>
#include <grx/basic_types.hpp>
#include <util/vec.hpp>

namespace grx::gl::details
{
bool uniform(uint program, int location, float v0) {
    glGetError();
    glProgramUniform1f(program, location, v0);
    return glGetError() == GL_NO_ERROR;
}

bool uniform(uint program, int location, float v0, float v1) {
    glGetError();
    glProgramUniform2f(program, location, v0, v1);
    return glGetError() == GL_NO_ERROR;
}

bool uniform(uint program, int location, float v0, float v1, float v2) {
    glGetError();
    glProgramUniform3f(program, location, v0, v1, v2);
    return glGetError() == GL_NO_ERROR;
}

bool uniform(uint program, int location, float v0, float v1, float v2, float v3) {
    glGetError();
    glProgramUniform4f(program, location, v0, v1, v2, v3);
    return glGetError() == GL_NO_ERROR;
}

bool uniform(uint program, int location, double v0) {
    glGetError();
    glProgramUniform1d(program, location, v0);
    return glGetError() == GL_NO_ERROR;
}

bool uniform(uint program, int location, double v0, double v1) {
    glGetError();
    glProgramUniform2d(program, location, v0, v1);
    return glGetError() == GL_NO_ERROR;
}

bool uniform(uint program, int location, double v0, double v1, double v2) {
    glGetError();
    glProgramUniform3d(program, location, v0, v1, v2);
    return glGetError() == GL_NO_ERROR;
}

bool uniform(uint program, int location, double v0, double v1, double v2, double v3) {
    glGetError();
    glProgramUniform4d(program, location, v0, v1, v2, v3);
    return glGetError() == GL_NO_ERROR;
}

bool uniform(uint program, int location, int v0) {
    glGetError();
    glProgramUniform1i(program, location, v0);
    return glGetError() == GL_NO_ERROR;
}

bool uniform(uint program, int location, int v0, int v1) {
    glGetError();
    glProgramUniform2i(program, location, v0, v1);
    return glGetError() == GL_NO_ERROR;
}

bool uniform(uint program, int location, int v0, int v1, int v2) {
    glGetError();
    glProgramUniform3i(program, location, v0, v1, v2);
    return glGetError() == GL_NO_ERROR;
}

bool uniform(uint program, int location, int v0, int v1, int v2, int v3) {
    glGetError();
    glProgramUniform4i(program, location, v0, v1, v2, v3);
    return glGetError() == GL_NO_ERROR;
}

bool uniform(uint program, int location, uint v0) {
    glGetError();
    glProgramUniform1ui(program, location, v0);
    return glGetError() == GL_NO_ERROR;
}

bool uniform(uint program, int location, uint v0, uint v1) {
    glGetError();
    glProgramUniform2ui(program, location, v0, v1);
    return glGetError() == GL_NO_ERROR;
}

bool uniform(uint program, int location, uint v0, uint v1, uint v2) {
    glGetError();
    glProgramUniform3ui(program, location, v0, v1, v2);
    return glGetError() == GL_NO_ERROR;
}

bool uniform(uint program, int location, uint v0, uint v1, uint v2, uint v3) {
    glGetError();
    glProgramUniform4ui(program, location, v0, v1, v2, v3);
    return glGetError() == GL_NO_ERROR;
}

#define UNIFORM_VEC_FUNC(type, num, glpostfix)                                                                         \
    bool uniform##num(uint program, int location, size_t count, type value) {                                          \
        glGetError();                                                                                                  \
        glProgramUniform##num##glpostfix(program, location, static_cast<GLsizei>(count), value);                       \
        return glGetError() == GL_NO_ERROR;                                                                            \
    }

#define GEN_UNIFORM_VEC_FUNCS(type, glpostfix)                                                                         \
    UNIFORM_VEC_FUNC(type, 1, glpostfix)                                                                               \
    UNIFORM_VEC_FUNC(type, 2, glpostfix)                                                                               \
    UNIFORM_VEC_FUNC(type, 3, glpostfix)                                                                               \
    UNIFORM_VEC_FUNC(type, 4, glpostfix)

GEN_UNIFORM_VEC_FUNCS(const float*, fv)
GEN_UNIFORM_VEC_FUNCS(const double*, dv)
GEN_UNIFORM_VEC_FUNCS(const int*, iv)
GEN_UNIFORM_VEC_FUNCS(const uint*, uiv)

#define UNIFORM_MAT_FUNC(type, matnum, glmatnum)                                                                       \
    bool uniform##matnum(uint program, int location, size_t count, bool transpose, type value) {                       \
        glGetError();                                                                                                  \
        glProgramUniformMatrix##glmatnum(                                                                              \
            program, location, static_cast<GLsizei>(count), static_cast<GLboolean>(transpose), value);                 \
        return glGetError() == GL_NO_ERROR;                                                                            \
    }

#define GEN_UNIFORM_MAT_FUNCS(type, typepostfix)                                                                       \
    UNIFORM_MAT_FUNC(type, 2x2, 2##typepostfix)                                                                        \
    UNIFORM_MAT_FUNC(type, 2x3, 2x3##typepostfix)                                                                      \
    UNIFORM_MAT_FUNC(type, 2x4, 2x4##typepostfix)                                                                      \
    UNIFORM_MAT_FUNC(type, 3x2, 3x2##typepostfix)                                                                      \
    UNIFORM_MAT_FUNC(type, 3x3, 3##typepostfix)                                                                        \
    UNIFORM_MAT_FUNC(type, 3x4, 3x4##typepostfix)                                                                      \
    UNIFORM_MAT_FUNC(type, 4x2, 4x2##typepostfix)                                                                      \
    UNIFORM_MAT_FUNC(type, 4x3, 4x3##typepostfix)                                                                      \
    UNIFORM_MAT_FUNC(type, 4x4, 4##typepostfix)

GEN_UNIFORM_MAT_FUNCS(const float*, fv)
GEN_UNIFORM_MAT_FUNCS(const double*, dv)

template <typename T, size_t S>
bool uniform(uint program, int location, const util::vec<T, S>& v) {
    static_assert(v.size() >= 2 && v.size() <= 4);

    if constexpr (v.size() == 2)
        return uniform(program, location, v.x(), v.y());
    else if constexpr (v.size() == 3)
        return uniform(program, location, v.x(), v.y(), v.z());
    else if constexpr (v.size() == 4)
        return uniform(program, location, v.x(), v.y(), v.z(), v.w());
}

template <core::number T>
bool uniform(uint program, int location, std::span<const T> v) {
    return uniform1(program, location, v.size(), v.data());
}

template <typename T, size_t S> requires (S == 2)
bool uniform(uint program, int location, std::span<const util::vec<T, S>> v) {
    static_assert(S >= 2 && S <= 4);

    if constexpr (S == 2)
        return uniform2(program, location, v.size(), v.data());
    else if constexpr (S == 3)
        return uniform3(program, location, v.size(), v.data());
    else if constexpr (S == 4)
        return uniform4(program, location, v.size(), v.data());
}

template <glm::length_t C, glm::length_t R, core::floating_point T, glm::qualifier Q>
bool uniform(uint program, int location, std::span<const glm::mat<C, R, T, Q>> m) {
    if constexpr (C == 2 && R == 2)
        return uniform2x2(program, location, static_cast<size_t>(m.size()), false, &(m.at(0)[0][0]));
    else if constexpr (C == 2 && R == 3)
        return uniform2x3(program, location, static_cast<size_t>(m.size()), false, &(m.at(0)[0][0]));
    else if constexpr (C == 2 && R == 4)
        return uniform2x4(program, location, static_cast<size_t>(m.size()), false, &(m.at(0)[0][0]));
    else if constexpr (C == 3 && R == 2)
        return uniform3x2(program, location, static_cast<size_t>(m.size()), false, &(m.at(0)[0][0]));
    else if constexpr (C == 3 && R == 3)
        return uniform3x3(program, location, static_cast<size_t>(m.size()), false, &(m.at(0)[0][0]));
    else if constexpr (C == 3 && R == 4)
        return uniform3x4(program, location, static_cast<size_t>(m.size()), false, &(m.at(0)[0][0]));
    else if constexpr (C == 4 && R == 2)
        return uniform4x2(program, location, static_cast<size_t>(m.size()), false, &(m.at(0)[0][0]));
    else if constexpr (C == 4 && R == 3)
        return uniform4x3(program, location, static_cast<size_t>(m.size()), false, &(m.at(0)[0][0]));
    else if constexpr (C == 4 && R == 4)
        return uniform4x4(program, location, static_cast<size_t>(m.size()), false, &(m.at(0)[0][0]));
}

template <glm::length_t C, glm::length_t R, core::floating_point T, glm::qualifier Q>
bool uniform(uint program, int location, const glm::mat<C, R, T, Q>& m) {
    return uniform(program, location, std::span(&m, 1));
}
} // namespace grx::gl::details
