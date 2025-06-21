#pragma once

#include <vector>
#include <glm/mat4x4.hpp>

#include <grx/basic_types.hpp>
#include <grx/bone_vertex.hpp>

namespace grx {
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
} // namespace grx
