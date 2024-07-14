#pragma once

#include <vector>

#include <grx/gl/gl_program.hpp>
#include <grx/gl/gl_render_targets.hpp>
#include <grx/gl/gl_vao.hpp>
#include <grx/postprocess.hpp>

#define fwd(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace grx::gl
{
template <gl_render_target_settings Settings, size_t MaxCallbackSize = 32>
class gl_postprocess_mgr {
public:
    using postprocess_t = postprocess<gl_program, MaxCallbackSize>;

    static inline constexpr core::int_const<size_t(0)> render_target       = {};
    static inline constexpr core::int_const<size_t(1)> source_target       = {};
    static inline constexpr size_t                     quad_vertices_count = 18;
    static inline constexpr uint                       texture_binding     = 0;

    gl_postprocess_mgr(vec2u texture_size): _targets(texture_size) {
        core::array data = {
            vec3f{-1.0f, -1.0f, 0.0f},
            vec3f{1.0f, -1.0f, 0.0f},
            vec3f{-1.0f, 1.0f, 0.0f},
            vec3f{-1.0f, 1.0f, 0.0f},
            vec3f{1.0f, -1.0f, 0.0f},
            vec3f{1.0f, 1.0f, 0.0f},
        };
        _quad.bind();
        _quad.set_vbo(0_zuc, data);
    }

    void add_postprocess(gl_program& program, auto&& update_callback) {
        _postprocesses.emplace_back(program, fwd(update_callback));
        _postprocesses.back().set_input_texture_id(texture_binding);
    }

private:
    void step(postprocess_t& postprocess) {
        _targets.bind_framebuffer_and_clear(render_target);

        _quad.bind();
        _targets.bind_texture(source_target, texture_binding);
        postprocess.update();

        _quad.draw(quad_vertices_count);

        _targets.swap_targets(render_target, source_target);
    }

private:
    gl_render_targets<Settings, Settings> _targets;
    gl_vao<vbo_vec3f>                     _quad;
    std::vector<postprocess_t>            _postprocesses;
};
} // namespace grx::gl

#undef fwd
