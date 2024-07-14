#pragma once

#include <GL/glew.h>

#include <core/array.hpp>
#include <core/moveonly_trivial.hpp>
#include <core/tuple.hpp>
#include <core/utility/int_seq.hpp>
#include <grx/basic_types.hpp>
#include <grx/texture.hpp>
#include <util/assert.hpp>

namespace grx::gl
{

enum class gl_color_fmt { rgb = 0, srgb, rgb16, rgb16f, rgb32f };
enum class gl_filtering { linear = 0, nearest };

struct gl_render_target_settings {
    gl_color_fmt color_fmt     = gl_color_fmt::rgb;
    gl_filtering filtering     = gl_filtering::linear;
    uint         mipmaps_count = 0;
};

namespace details
{
    inline auto to_gl(gl_color_fmt fmt) {
        return core::array{GL_RGB, GL_SRGB, GL_RGB16, GL_RGB16F, GL_RGB32F}[uint(fmt)];
    }

    inline auto to_gl(gl_filtering filtering) {
        return core::array{GL_LINEAR, GL_NEAREST}[uint(filtering)];
    }
} // namespace details

template <gl_render_target_settings... cfg>
class gl_render_targets {
public:
    using gl_id_t = core::moveonly_trivial<uint, uint(-1)>;

    gl_render_targets(vec2u size): _size(size) {
        glCreateFramebuffers(GLsizei(targets_count()), (uint*)_framebuffers.data());
        glCreateRenderbuffers(GLsizei(targets_count()), (uint*)_depthbuffers.data());
        glCreateTextures(GL_TEXTURE_2D, GLsizei(targets_count()), (uint*)_textures.data());

        util::glog().debug("[gl_render_target]: init framebuffers: {}", _framebuffers);
        util::glog().debug("[gl_render_target]: init depthbuffers: {}", _depthbuffers);
        util::glog().debug("[gl_render_target]: init textures: {}", _textures);

        static constexpr auto settings = core::tuple{cfg...};

        core::make_idx_seq<targets_count()>().foreach([this](auto i) {
            glTextureStorage2D(_textures[i],
                               GLsizei(settings[i].mipmaps_count + 1),
                               details::to_gl(settings[i].color_fmt),
                               GLsizei(_size.x()),
                               GLsizei(_size.y()));

            glTextureParameteri(_textures[i], GL_TEXTURE_MAG_FILTER, details::to_gl(settings[i].filtering));
            glTextureParameteri(_textures[i], GL_TEXTURE_MIN_FILTER, details::to_gl(settings[i].filtering));
            glTextureParameteri(_textures[i], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTextureParameteri(_textures[i], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glNamedRenderbufferStorage(_depthbuffers[i], GL_DEPTH_COMPONENT24, GLsizei(_size.x()), GLsizei(_size.y()));
            glNamedFramebufferRenderbuffer(_framebuffers[i], GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depthbuffers[i]);
            glNamedFramebufferTexture(_framebuffers[i], GL_COLOR_ATTACHMENT0, _textures[i], 0);
            glNamedFramebufferDrawBuffer(_framebuffers[i], GL_COLOR_ATTACHMENT0);

            R_ASSERT(glCheckNamedFramebufferStatus(_framebuffers[i], GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
                     "Error while creating render target");
        });
    }

    ~gl_render_targets() {
        if (!_framebuffers.front()) {
            glDeleteFramebuffers(GLsizei(targets_count()), (uint*)_framebuffers.data());
            glDeleteRenderbuffers(GLsizei(targets_count()), (uint*)_depthbuffers.data());
            glDeleteTextures(GLsizei(targets_count()), (uint*)_textures.data());
        }
    }

    constexpr vec2u size() const {
        return _size;
    }

    static constexpr size_t targets_count() {
        return sizeof...(cfg);
    }

    void swap_targets(auto target_num1, auto target_num2) {
        std::swap(_framebuffers[target_num1], _framebuffers[target_num2]);
        std::swap(_depthbuffers[target_num1], _depthbuffers[target_num2]);
        std::swap(_textures[target_num1], _textures[target_num2]);
    }

    void bind_texture(auto target_num, uint unit_number) {
        /* TODO: check for < 32 (48) */
        glBindTextureUnit(unit_number, _textures[target_num]);
    }

    void bind_framebuffer(auto target_num) {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _framebuffers[target_num]);
        glViewport(0, 0, GLsizei(_size.x()), GLsizei(_size.y()));
    }

    void bind_framebuffer_and_clear(auto target_num) {
        bind_framebuffer(target_num);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void redraw_mipmaps(auto target_num) {
        glGenerateTextureMipmap(_textures[target_num]);
    }

private:
    core::array<gl_id_t, targets_count()> _framebuffers;
    core::array<gl_id_t, targets_count()> _depthbuffers;
    core::array<gl_id_t, targets_count()> _textures;
    vec2u                                 _size;
};
} // namespace grx::gl
