#pragma once

#include <GL/glew.h>

#include <core/int_const.hpp>
#include <core/meta/type_list.hpp>

#include <grx/basic_types.hpp>
#include <grx/gl/gl_ctx.hpp>
#include <grx/gl/gl_vbo.hpp>
#include <grx/gl/glfw_ctx_mapper.hpp>

namespace grx::gl
{

namespace details
{
    template <typename... Ts>
    class gl_vao_indices {
    public:
        void draw(size_t vertex_count, size_t start_vertex_pos = 0) {
            glDrawArrays(GL_TRIANGLES, GLint(start_vertex_pos), GLsizei(vertex_count));
        }
    };

    template <typename... Ts> requires core::any_of<vbo_index, Ts...>
    class gl_vao_indices<Ts...> {
    public:
        uint indices_count(this const auto& it) {
            using core::type, core::type_list;
            return it.vbo_sizes[type_list<Ts...>[type<vbo_index>]];
        }

        void draw(size_t indices_count, size_t start_index_pos = 0, size_t start_vertex_pos = 0) {
            glDrawElementsBaseVertex(GL_TRIANGLES,
                                     GLsizei(indices_count),
                                     GL_UNSIGNED_INT,
                                     (GLvoid*)(sizeof(uint) * start_index_pos), // NOLINT
                                     GLint(start_vertex_pos));
        }

        void draw_instanced(size_t instances_count,
                            size_t indices_count,
                            size_t start_index_pos  = 0,
                            size_t start_vertex_pos = 0) {
            glDrawElementsInstancedBaseVertex(GL_TRIANGLES,
                                              GLsizei(indices_count),
                                              GL_UNSIGNED_INT,
                                              (GLvoid*)(sizeof(uint) * start_index_pos), // NOLINT
                                              GLsizei(instances_count),
                                              GLint(start_vertex_pos));
        }
    };
} // namespace details

template <vbo_data... Ts>
class gl_vao : public glfw_ctx_mapper<uint>, public details::gl_vao_indices<Ts...> {
public:
    static uint new_gl_vao() {
        uint vao;
        glGenVertexArrays(1, &vao);
        return vao;
    }

    static constexpr size_t size() {
        return sizeof...(Ts);
    }

    static constexpr auto _location_iter(auto i, auto idx, auto location) {
        using core::type, core::type_list, core::int_c;

        if constexpr (idx == i)
            return location;
        else {
            if constexpr (type_list<Ts...>[idx] == type<vbo_index>)
                return _location_iter(idx + int_c<1>, location);
            else if constexpr (type_list<Ts...>[idx] == type<vbo_mat4>)
                return _location_iter(idx + int_c<1>, location + int_c<4>);
            else if constexpr (type_list<Ts...>[idx] == type<vbo_bone_vertex4>)
                return _location_iter(idx + int_c<1>, location + int_c<2>);
            else
                return _location_iter(idx + int_c<1>, location + int_c<1>);
        }
    }

    template <size_t I>
    static constexpr uint location_of(core::int_const<I> i = {}) {
        using core::int_c;
        return _location_iter(i, int_c<uint(0)>, int_c<uint(0)>);
    }

    static constexpr bool have_indices() {
        return core::any_of<vbo_index, Ts...>;
    }

    uint new_glfw_ctx_handler(GLFWwindow*) {
        auto vao_id = new_gl_vao();
        rebind_buffers(vao_id);
        return vao_id;
    }

    void rebind_buffers(uint vao_id) {
        using core::type_list, core::decl_type, core::int_c;

        ctx().bind_vao(vao_id);

        if (!vbos_initialized) {
            glGenBuffers(GLsizei(size()), vbo_ids.data());
            vbos_initialized = true;
        }

        type_list<Ts...>.indexed().foreach ([this](auto t) {
            using element_t = typename decl_type<t.type()>::value_type;
            vbo_functions<element_t>::bind(vbo_ids[t.index()], location_of(t.index()));
        });
    }

    void bind() {
        auto vao_id = get_current_ctx_gl_id();
        ctx().bind_vao(vao_id);
    }

    template <size_t I>
    void bind_vbo(core::int_const<I> i, uint gl_target) {
        glBindBuffer(gl_target, vbo_ids[i]);
    }

    template <size_t I>
    void set_vbo(core::int_const<I> i, auto&& vbo) {
        using element_t = decltype(core::type_list<typename Ts::value_type...>[core::int_c<I>])::type;
        vbo_functions<element_t>::set(vbo_ids[i], location_of(i), std::span<const element_t>(vbo));
        vbo_sizes[i] = uint(vbo.size());
    }

    template <size_t I>
    void get_vbo(core::int_const<I> i) const {
        using core::decl_type, core::type_list;
        return vbo_functions<decl_type<type_list<Ts...>[i]>>::get(vbo_ids[i], vbo_sizes[i]);
    }

    template <size_t I>
    uint get_vbo_size(core::int_const<I> i) const {
        return vbo_sizes[i];
    }

private:
    core::array<uint, size()> vbo_ids;
    core::array<uint, size()> vbo_sizes = {0};
    bool vbos_initialized = false;
};
} // namespace grx::gl
