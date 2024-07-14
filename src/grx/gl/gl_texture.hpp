#pragma once

#include <GL/glew.h>

#include <core/concepts/same_as.hpp>
#include <core/exception.hpp>
#include <core/moveonly_trivial.hpp>

#include <grx/texture.hpp>

namespace grx::gl {
class gl_texture_exception : public core::exception {};

class gl_texture_invalid_components_count : public gl_texture_exception {
public:
    gl_texture_invalid_components_count(uint components_count):
        msg("Invalid components count: " + std::to_string(components_count)) {}

    const char* what() const noexcept override {
        return msg.data();
    }

private:
    std::string msg;
};

template <typename T>
class gl_texture {
public:
    using gl_id_t = core::moveonly_trivial<uint, uint(-1)>;

    static GLenum gl_component_internal_format() {
        switch (components_count<T>()) {
        case 1: return have_component_type<T, float> ? GL_R16F : GL_R8;
        case 2: return have_component_type<T, float> ? GL_RG16F : GL_RG8;
        case 3: return have_component_type<T, float> ? GL_RGB16F : GL_RGB8;
        case 4: return have_component_type<T, float> ? GL_RGBA16F : GL_RGBA8;
        default: throw gl_texture_invalid_components_count(components_count<T>());
        }
    }

    static GLenum gl_component_format() {
        switch (components_count<T>()) {
        case 1: return GL_RED;
        case 2: return GL_RG;
        case 3: return GL_RGB;
        case 4: return GL_RGBA;
        default: throw gl_texture_invalid_components_count(components_count<T>());
        }
    }

    static constexpr GLenum gl_component_type() {
        if constexpr (have_component_type<T, float>)
            return GL_FLOAT;
        else
            return GL_UNSIGNED_BYTE;
    }

    gl_texture() = default;

    gl_texture(vec2u size, uint mipmaps_count): _mipmaps_count(mipmaps_count), _size(size) {
        /* TODO: check for size */
        create();
    }

    ~gl_texture() {
        destroy();
    }

    void set(const texture<T>& texture) {
        /* TODO: check for size */
        if (_size != texture.size || !_id) {
            destroy();
            _size          = texture.size();
            _mipmaps_count = texture.mipmaps_count();
            create();
        }

        for (int level = 0; auto&& mipmap : texture.mipmaps())
            glTextureSubImage2D(_id,
                                level++,
                                0,
                                0,
                                GLsizei(mipmap.size().x()),
                                GLsizei(mipmap.size().y()),
                                gl_component_format(),
                                gl_component_type(),
                                mipmap.data());

        /* TODO: make setting for this */
        glTextureParameteri(_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    }

    texture<T> get() const {
        texture<T> result{_size, _mipmaps_count};

        for (int level = 0; auto&& mipmap : result.mipmaps())
            glGetTextureImage(_id,
                              level++,
                              gl_component_format(),
                              gl_component_type(),
                              GLsizei(mipmap.size().x() * mipmap.size().y() * sizeof(T)),
                              mipmap.data());

        return result;
    }

    void bind(uint unit_number) {
        /* TODO: check for < 32 (48) */
        glBindTextureUnit(unit_number, _id);
    }

    uint id() const {
        return _id;
    }

    vec2u size() const {
        return _size;
    }

    uint mipmaps_count() const {
        return _mipmaps_count;
    }

private:
    void create() {
        glCreateTextures(GL_TEXTURE_2D, 1, &_id.get());
        glTextureStorage2D(_id,
                           int(_mipmaps_count + 1),
                           gl_component_internal_format(),
                           GLsizei(_size.x()),
                           GLsizei(_size.y()));
    }

    void destroy() {
        if (_id)
            glDeleteTextures(1, &_id.get());
        _id.reset();
    }

private:
    gl_id_t _id;
    uint    _mipmaps_count = 0;
    vec2u   _size          = {0, 0};
};
}
