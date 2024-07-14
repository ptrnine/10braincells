#pragma once

#include <vector>

#include <GL/glew.h>
#include <glm/mat4x4.hpp>

#include <core/concepts/any_of.hpp>
#include <grx/bone_vertex.hpp>
#include <util/vec.hpp>

namespace grx::gl
{
using vbo_vec2f        = std::vector<vec2f>;
using vbo_vec3f        = std::vector<vec3f>;
using vbo_index        = std::vector<uint>;
using vbo_mat4         = std::vector<glm::mat4>;
using vbo_bone_vertex4 = std::vector<bone_vertex4>;

namespace details {
    template <typename T>
    struct _vbo_element_type;

    template <typename T>
    struct _vbo_element_type<std::vector<T>> {
        using type = T;
    };

    template <typename T>
    struct _vbo_element_type<std::span<T>> {
        using type = core::remove_const<T>;
    };
}

template <typename T>
using vbo_element_type = typename details::_vbo_element_type<T>::type;

template <typename T>
concept vbo_element = core::any_of<core::remove_const<T>, vec2f, vec3f, uint, glm::mat4, bone_vertex4>;

template <typename T>
concept vbo_data = core::any_of<T, vbo_vec2f, vbo_vec3f, vbo_index, vbo_mat4, vbo_bone_vertex4>;

template <typename T>
struct vbo_functions_base {
    std::vector<T> get(uint vbo_id, uint size) {
        std::vector<T> result(size);
        glGetNamedBufferSubData(vbo_id, 0, GLsizeiptr(size * sizeof(T)), result.data());
        return result;
    }
};

template <typename T>
struct vbo_functions;

template <>
struct vbo_functions<vec2f> : vbo_functions_base<vec2f> {
    static void bind(uint vbo_id, uint location) {
        glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
        glEnableVertexAttribArray(location);
        glVertexAttribPointer(location, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    }

    static void set(uint vbo_id, uint location, std::span<const vec2f> data) {
        glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
        glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(sizeof(vec2f) * data.size()), data.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(location);
        glVertexAttribPointer(location, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    }
};

template <>
struct vbo_functions<vec3f> : vbo_functions_base<vec3f> {
    static void bind(uint vbo_id, uint location) {
        glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
        glEnableVertexAttribArray(location);
        glVertexAttribPointer(location, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    }

    static void set(uint vbo_id, uint location, std::span<const vec3f> data) {
        glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
        glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(sizeof(vec3f) * data.size()), data.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(location);
        glVertexAttribPointer(location, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    }
};

template <>
struct vbo_functions<uint> : vbo_functions_base<uint> {
    static void bind(uint vbo_id, uint /*location*/) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_id);
    }

    static void set(uint vbo_id, uint /*location*/, std::span<const uint> data) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_id);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, GLsizeiptr(sizeof(uint) * data.size()), data.data(), GL_STATIC_DRAW);
    }
};

template <>
struct vbo_functions<glm::mat4> : vbo_functions_base<glm::mat4> {
    static void bind(uint vbo_id, uint location) {
        glBindBuffer(GL_ARRAY_BUFFER, vbo_id);

        for (uint i = 0; i < 4; ++i) {
            glEnableVertexAttribArray(location + i);
            glVertexAttribPointer(location + i,
                                  4,
                                  GL_FLOAT,
                                  GL_FALSE,
                                  sizeof(glm::mat4),
                                  (void*)(sizeof(GLfloat) * i * 4)); // NOLINT
            glVertexAttribDivisor(location + i, 1);
        }
    }

    static void set(uint vbo_id, uint /*location*/, std::span<const glm::mat4> data) {
        glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
        glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(sizeof(glm::mat4) * data.size()), data.data(), GL_STATIC_DRAW);
    }
};

template <>
struct vbo_functions<bone_vertex4> : vbo_functions_base<bone_vertex4> {
    static inline constexpr size_t bone_per_vertex = bone_vertex4::size();

    static void bind(uint vbo_id, uint location) {
        glBindBuffer(GL_ARRAY_BUFFER, vbo_id);

        glEnableVertexAttribArray(location);
        glVertexAttribIPointer(location,
                               GLint(bone_per_vertex),
                               GL_INT,
                               GLsizei((sizeof(uint) + sizeof(float)) * bone_per_vertex),
                               nullptr);

        glEnableVertexAttribArray(location + 1);
        glVertexAttribPointer(location + 1,
                              GLint(bone_per_vertex),
                              GL_FLOAT,
                              GL_FALSE,
                              GLsizei((sizeof(uint) + sizeof(float)) * bone_per_vertex),
                              (void*)(sizeof(uint) * bone_per_vertex)); // NOLINT
    }

    static void set(uint vbo_id, uint location, std::span<const bone_vertex4> data) {
        glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
        glBufferData(GL_ARRAY_BUFFER,
                     GLsizeiptr((sizeof(uint) + sizeof(float)) * bone_per_vertex * data.size()),
                     data.data(),
                     GL_STATIC_DRAW);

        glEnableVertexAttribArray(location);
        glVertexAttribIPointer(location,
                               GLint(bone_per_vertex),
                               GL_INT,
                               GLsizei((sizeof(uint) + sizeof(float)) * bone_per_vertex),
                               nullptr);

        glEnableVertexAttribArray(location + 1);
        glVertexAttribPointer(location + 1,
                              GLint(bone_per_vertex),
                              GL_FLOAT,
                              GL_FALSE,
                              GLsizei((sizeof(uint) + sizeof(float)) * bone_per_vertex),
                              (void*)(sizeof(uint) * bone_per_vertex)); // NOLINT
    }
};
} // namespace grx::gl
