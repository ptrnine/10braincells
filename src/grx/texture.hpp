#pragma once

#ifdef DISABLE_SINGLE_OBJECT_BUILD
#include <stb_image.h>
#include <stb_image_resize2.h>
#include <stb_image_write.h>
#else
#include <grx/stb_sources.hpp>
#endif

#include <core/bit.hpp>
#include <core/box.hpp>
#include <core/concepts/number.hpp>
#include <core/concepts/trivial_ctor.hpp>
#include <core/concepts/trivial_dtor.hpp>
#include <core/finalizer.hpp>
#include <core/ranges/zip.hpp>
#include <core/tuple.hpp>
#include <grx/color.hpp>
#include <util/log.hpp>
#include <util/print.hpp>
#include <util/ranges/dimensional_seq.hpp>
#include <util/math.hpp>

namespace grx
{
using core::unsigned_integral;
using core::same_as;
using core::floating_point;
using core::malloc_box;
using core::malloc_boxed;

namespace constants
{
    constexpr u32   default_mipmaps_count = 8;
    constexpr vec2u min_mipmap_size       = {2, 2};
} // namespace constants

template <typename T>
inline vec2<T> clamp_mipmap_size(vec2<T> value) {
    if (value.x() < T(constants::min_mipmap_size.x()))
        value.x() = T(constants::min_mipmap_size.x());
    if (value.y() < T(constants::min_mipmap_size.y()))
        value.y() = T(constants::min_mipmap_size.y());
    return value;
}

template <typename T> requires (core::trivial_ctor<T> && core::trivial_dtor<T>)
class texture_span;

    template <typename T> requires (core::trivial_ctor<T> && core::trivial_dtor<T>)
class texture;

class stbi_write_image_error : public core::exception {
public:
    stbi_write_image_error(std::string message): msg(core::mov(message)) {}

    const char* what() const noexcept override {
        return msg.data();
    }

private:
    std::string msg;
};

class stbi_resize_image_error : public core::exception {
public:
    stbi_resize_image_error(std::string message): msg(core::mov(message)) {}

    const char* what() const noexcept override {
        return msg.data();
    }

private:
    std::string msg;
};

class stbi_read_image_error : public core::exception {
public:
    stbi_read_image_error(std::string message): msg(core::mov(message)) {}

    const char* what() const noexcept override {
        return msg.data();
    }

private:
    std::string msg;
};

namespace details {
    template <typename T>
    class texture_base {
    public:
        static constexpr stbir_pixel_layout stbi_pixel_layout() {
            switch (components_count<T>()) {
            case 1: return STBIR_1CHANNEL;
            case 2: return STBIR_2CHANNEL;
            case 3: return STBIR_RGB;
            case 4: return STBIR_RGBA;
            }
        }

        static constexpr stbir_datatype stbi_data_type() {
            constexpr auto get_type = [](auto type) {
                if constexpr (type == core::type<u8>)
                    return STBIR_TYPE_UINT8;
                else if constexpr (type == core::type<float>)
                    return STBIR_TYPE_FLOAT;
            };

            if constexpr (util::math_vector<T>)
                return get_type(core::type<typename T::value_type>);
            else
                return get_type(core::type<T>);
        }

        static constexpr size_t mipmaps_region_len(const vec2u& size, size_t mipmaps_count) {
            size_t result      = 0;
            auto   mipmap_size = size / 2;

            for (size_t i = 0; i < mipmaps_count; ++i) {
                result += mipmap_size.x() * mipmap_size.y();
                mipmap_size = clamp_mipmap_size(mipmap_size / 2);
            }

            return result;
        }

        static constexpr void generate_mipmaps(T* image, const vec2u& size, size_t mipmaps_count) {
            auto read_pos   = image;
            auto read_size  = vec2i{size};

            STBIR_RESIZE stbi_resize;
            for (size_t i = 0; i < mipmaps_count; ++i) {
                auto write_pos  = read_pos + read_size.x() * read_size.y();
                auto write_size = clamp_mipmap_size(read_size / 2);

                stbir_resize_init(&stbi_resize,
                                  read_pos,
                                  read_size.x(),
                                  read_size.y(),
                                  0,
                                  write_pos,
                                  write_size.x(),
                                  write_size.y(),
                                  0,
                                  stbi_pixel_layout(),
                                  stbi_data_type());

                if (!stbir_resize_extended(&stbi_resize))
                    throw stbi_resize_image_error("stbir_resize_extended() failed");

                read_pos = write_pos;
                read_size = write_size;
            }
        }
    };

    struct texture_pixel_iterator_sentinel {};

    template <typename T>
    class texture_pixel_iterator {
    public:
        texture_pixel_iterator(T* data, vec2u left_top, vec2u size, vec2u owner_size):
            _data(data), _left_top(left_top), _owner_size(owner_size), _current(size.v) {}

        texture_pixel_iterator& operator++() {
            ++_current;
            return *this;
        }

        texture_pixel_iterator operator++(int) {
            auto res = *this;
            ++(*this);
            return res;
        }

        texture_pixel_iterator& operator--() {
            --_current;
            return *this;
        }

        texture_pixel_iterator operator--(int) {
            auto res = *this;
            --(*this);
            return res;
        }

        bool operator==(const texture_pixel_iterator& i) const {
            return _current == i._current && _data == i._data && _left_top == i._left_top &&
                   _owner_size == i._owner_size;
        }

        bool operator!=(const texture_pixel_iterator& i) const {
            return !(*this == i);
        }

        bool operator==(texture_pixel_iterator_sentinel) const {
            return _current == util::dimensional_index_iterator_sentinel{};
        }

        bool operator!=(texture_pixel_iterator_sentinel) const {
            return _current != util::dimensional_index_iterator_sentinel{};
        }

        T& operator*() const {
            auto [w, h] = *_current;
            return *(_data + (_left_top.y() + h) * _owner_size.x() + _left_top.x() + w);
        }

        T* operator->() const {
            return &operator*();
        }

        T* data() const {
            return _data;
        }

        vec2u left_top() const {
            return _left_top;
        }

        vec2u owner_size() const {
            return _owner_size;
        }

    private:
        T*                                        _data;
        vec2u                                     _left_top;
        vec2u                                     _owner_size;
        util::dimensional_index_iterator<uint, 2> _current;
    };

    template <typename T>
    class texture_pixel_span {
    public:
        texture_pixel_span(T* data, vec2u left_top, vec2u size, vec2u owner_size):
            _b(data, left_top, size, owner_size) {}

        auto begin() const {
            return _b;
        }

        texture_pixel_iterator_sentinel end() const {
            return {};
        }

        T* operator[](uint y_idx) const {
            return _b.data() + (_b.left_top().y() + y_idx) * _b.owner_size().x();
        }

        T& operator[](uint x_idx, uint y_idx) const {
            return *(operator[](y_idx) + _b.left_top().x() + x_idx);
        }

        T& operator[](vec2u idxs) const {
            return operator[](idxs.x(), idxs.y());
        }

    private:
        texture_pixel_iterator<T> _b;
    };

    void stbi_write_callback(void* byte_vector, void* data, int size) {
        auto vec      = static_cast<std::vector<byte>*>(byte_vector);
        auto old_size = vec->size();
        vec->resize(old_size + size_t(size));
        std::memcpy(vec->data() + old_size, data, static_cast<size_t>(size)); // NOLINT
    }

    template <typename T>
    class texture_span_u8_stbi : public texture_base<T> {
    public:
        auto to_ldr(this auto& it, float gamma = 1.f) {
            using pixel_t = component_type_cast<u8, T>;
            texture<pixel_t> result{it.size(), it.mipmaps_count()};

            for (auto&& [dst, src] : core::zip(result.pixels(), it.pixels()))
                dst = pixel_t(grayscale<T>(255) * pow(src, 1.f / gamma));

            result.generate_mipmaps();
            return result;
        }

        auto to_ldr(this auto& it, float gamma, float exposure) {
            using pixel_t = component_type_cast<u8, T>;
            texture<pixel_t> result{it.size(), it.mipmaps_count()};

            for (auto&& [dst, src] : core::zip(result.pixels(), it.pixels())) {
                auto color = grayscale<T>(1) - exp(-src * exposure);
                dst = pixel_t(grayscale<T>(255) * pow(color, 1.f / gamma));
            }

            result.generate_mipmaps();
            return result;
        }
    };

    template <have_component_type<u8> T>
    class texture_span_u8_stbi<T> : public texture_base<T> {
    public:
        std::vector<core::byte> to_png(this const auto& it) {
            std::vector<byte> result;

            auto data_start = it.data() + it.left_top().y() * it.owner_size().x() + it.left_top().x();
            auto stride     = it.owner_size().x() * sizeof(T);

            auto [w, h] = vec2i{it.size()};
            auto ok     = stbi_write_png_to_func(
                details::stbi_write_callback, &result, w, h, int(components_count<T>()), data_start, int(stride));

            if (!ok)
                throw stbi_write_image_error("Cannot convert texture to png image");

            return result;
        }
    };

    /* TODO: handle non-vec types (single float/u8) */
    template <typename T>
    class texture_span_stbi : public texture_span_u8_stbi<T> {
    public:
        using base = texture_base<T>;

        auto pixels(this auto&& it) {
            return texture_pixel_span{it.data(), it.left_top(), it.size(), it.owner_size()};
        }

        auto crop(this auto&& it, vec2u left_top, vec2u size) {
            return texture_span{
                it.data(),
                it.left_top() + left_top,
                size,
                it.owner_size(),
            };
        }

        auto crop(this auto&& it, vec2u left_top) {
            return texture_span{
                it.data(),
                it.left_top() + left_top,
                it.size() - left_top,
                it.owner_size(),
            };
        }

        void resize_to_memory(this const auto& it,
                              T*             output,
                              const vec2u&     new_size,
                              size_t           new_mipmaps_count      = 0,
                              size_t           new_mipmaps_region_len = 0) {
            auto mipmaps_region_len = new_mipmaps_region_len;
            if (mipmaps_region_len == 0 && new_mipmaps_count > 0)
                mipmaps_region_len = base::mipmaps_region_len(new_size, new_mipmaps_count);

            auto data_start   = it.data() + it.left_top().y() * it.owner_size().x() + it.left_top().x();
            auto stride       = it.owner_size().x() * sizeof(T);

            STBIR_RESIZE stb_resize;
            stbir_resize_init(&stb_resize,
                              data_start,
                              int(it.size().x()),
                              int(it.size().y()),
                              int(stride),
                              output,
                              int(new_size.x()),
                              int(new_size.y()),
                              0,
                              base::stbi_pixel_layout(),
                              base::stbi_data_type());

            if (!stbir_resize_extended(&stb_resize))
                throw stbi_resize_image_error("stbir_resize_extended() failed");

            if (new_mipmaps_count > 0)
                base::generate_mipmaps(output, new_size, new_mipmaps_count);
        }

        texture<T> resized(this const auto& it, const vec2u& new_size, size_t new_mipmaps_count = 0) {
            auto mipmaps_region_len = base::mipmaps_region_len(new_size, new_mipmaps_count);
            auto output_pixels      = malloc_boxed<T[]>(new_size.x() * new_size.y() + mipmaps_region_len);

            it.resize_to_memory(output_pixels.get(), new_size, new_mipmaps_count, mipmaps_region_len);

            return {
                core::mov(output_pixels),
                new_size,
                new_mipmaps_count,
                new_size.x() * new_size.y() + mipmaps_region_len,
            };
        }
    };

    template <typename T>
    class texture_stbi : public texture_span_stbi<T> {};

    template <have_component_type<u8> T>
    class texture_stbi<T> : public texture_span_stbi<T> {
    public:
        using base = texture_base<T>;

        std::vector<core::byte> to_tga(this const auto& it) {
            std::vector<byte> result;
            auto [w, h] = vec2i{it.size()};
            auto ok     = stbi_write_tga_to_func(
                details::stbi_write_callback, &result, w, h, int(components_count<T>()), it.data());
            if (!ok)
                throw stbi_write_image_error("Cannot convert texture to tga image");
            return result;
        }

        std::vector<core::byte> to_bmp(this const auto& it) {
            std::vector<byte> result;
            auto [w, h] = vec2i{it.size()};
            auto ok     = stbi_write_bmp_to_func(
                details::stbi_write_callback, &result, w, h, int(components_count<T>()), it.data());
            if (!ok)
                throw stbi_write_image_error("Cannot convert texture to tga image");
            return result;
        }

        std::vector<core::byte> to_jpg(this const auto& it, int quality = 95) {
            std::vector<byte> result;
            auto [w, h] = vec2i{it.size()};
            auto ok     = stbi_write_jpg_to_func(
                details::stbi_write_callback, &result, w, h, int(components_count<T>()), it.data(), quality);
            if (!ok)
                throw stbi_write_image_error("Cannot convert texture to tga image");
            return result;
        }

        static texture<T> from_image(std::span<const core::byte> image, uint mipmaps_count = 0) {
            constexpr int type = [] {
                switch (components_count<T>()) {
                case 1:
                    return STBI_grey;
                case 2:
                    return STBI_grey_alpha;
                case 3:
                    return STBI_rgb;
                case 4:
                    return STBI_grey_alpha;
                }
            }();

            vec2i size;
            int   comp_count;

            malloc_box<T[]> result{
                (T*)stbi_load_from_memory(     // NOLINT
                    (const u8*)(image.data()), // NOLINT
                    int(image.size()),
                    &size.x(),
                    &size.y(),
                    &comp_count,
                    type),
            };

            if (!result)
                throw stbi_read_image_error("Cannot convert image to texture");

            if (comp_count != components_count<T>())
                throw stbi_read_image_error(
                    util::format("Invalid channels count: required={} actual={}", components_count<T>(), comp_count));

            auto overal_len = size_t(size.x() * size.y());

            if (mipmaps_count > 0) {
                overal_len += base::mipmaps_region_len(vec2u{size}, mipmaps_count);
                result.replace((T*)std::realloc(result.get(), overal_len * sizeof(T))); // NOLINT
                base::generate_mipmaps(result.get(), vec2u{size}, mipmaps_count);
            }

            return {core::mov(result), vec2u{size}, mipmaps_count, overal_len};
        }
    };
}

template <typename T> requires (core::trivial_ctor<T> && core::trivial_dtor<T>)
class texture_span : public details::texture_span_stbi<T> {
public:
    constexpr texture_span(T* start, vec2u left_top, vec2u size, vec2u owner_size):
        _start(start), _left_top(left_top), _size(size), _owner_size(owner_size) {}

    constexpr auto data(this auto& it) {
        return it._start;
    }

    constexpr vec2u size() const {
        return _size;
    }

    constexpr vec2u left_top() const {
        return _left_top;
    }

    constexpr vec2u owner_size() const {
        return _owner_size;
    }

private:
    T*    _start;
    vec2u _left_top;
    vec2u _size;
    vec2u _owner_size;
};

struct texture_mipmap_iterator_sentinel {};

template <typename T>
class texture_mipmap_iterator {
public:
    /* mipmaps_count + 1 because we take the whole texture into account */
    texture_mipmap_iterator(T* data, vec2u size, uint start_mipmap, uint mipmaps_count):
        _start(data), _size(size), _mipmaps_count(mipmaps_count + 1) {
        if (start_mipmap > _mipmaps_count - 1)
            start_mipmap = _mipmaps_count - 1;

        for (uint i = 0; i < start_mipmap; ++i)
            next_mipmap();
    }

    texture_mipmap_iterator& operator++() {
        next_mipmap();
        return *this;
    }

    texture_mipmap_iterator operator++(int) {
        auto res = *this;
        next_mipmap();
        return res;
    }

    bool operator==(texture_mipmap_iterator_sentinel) const {
        return _current_mipmap == _mipmaps_count;
    }

    bool operator!=(texture_mipmap_iterator_sentinel sentinel) const {
        return !(*this == sentinel);
    }

    texture_span<T> operator*() const {
        return {_start, {0, 0}, _size, _size};
    }

    T* data() const {
        return _start;
    }

private:
    void next_mipmap() {
        _start += _size.x() * _size.y();
        _size = clamp_mipmap_size(_size / 2);
        ++_current_mipmap;
    }

private:
    T*    _start;
    vec2u _size;
    uint  _current_mipmap = 0;
    uint  _mipmaps_count;
};

template <typename T>
class texture_mipmap_span {
public:
    texture_mipmap_span(T* data, vec2u size, uint start_mipmap, uint mipmaps_count):
        _b(data, size, start_mipmap, mipmaps_count) {}

    texture_mipmap_iterator<T> begin() const {
        return _b;
    }

    texture_mipmap_iterator_sentinel end() const {
        return {};
    }

private:
    texture_mipmap_iterator<T> _b;
};

template <typename T> requires (core::trivial_ctor<T> && core::trivial_dtor<T>)
class texture : public details::texture_stbi<T> {
public:
    using base = details::texture_base<T>;

    friend class details::texture_span_stbi<T>;
    friend class details::texture_stbi<T>;

    texture(const vec2u& size, uint mipmaps_count = 0):
        _size(vec2<u32>{size}),
        _mipmaps_count(mipmaps_count),
        _overal_len(size.x() * size.y() + base::mipmaps_region_len(size, mipmaps_count)),
        _pixels(malloc_boxed<T[]>(_overal_len)) {}

    texture(const texture& t):
        _size(t._size),
        _mipmaps_count(t._mipmaps_count),
        _overal_len(t._overal_len),
        _pixels(malloc_boxed<T[]>(_overal_len)) {
        std::memcpy(_pixels.get(), t._pixels.get(), _overal_len * sizeof(T));
    }

    texture& operator=(const texture& t) {
        if (&t == this)
            return *this;

        _size          = t._size;
        _mipmaps_count = t._mipmaps_count;
        _overal_len    = t._overal_len;
        _pixels        = malloc_boxed<T[]>(_overal_len);

        std::memcpy(_pixels.get(), t._pixels.get(), _overal_len * sizeof(T));

        return *this;
    }

    texture(texture&&) noexcept = default;
    texture& operator=(texture&&) noexcept = default;

    auto begin(this auto& it) {
        if constexpr (core::is_const<core::remove_ref<decltype(it)>>)
            return (const T*)it._pixels.get();
        else
            return it._pixels.get();
    }

    auto end(this auto& it) {
        return it.begin() + it._overal_len;
    }

    constexpr vec2u left_top() const {
        return {0, 0};
    }

    constexpr vec2u owner_size() const {
        return _size;
    }

    constexpr u32 mipmaps_count() const {
        return _mipmaps_count;
    }

    constexpr vec2<u32> size() const {
        return _size;
    }

    constexpr auto data(this auto& it) {
        return it._pixels.get();
    }

    void generate_mipmaps() {
        if (_mipmaps_count)
            return;
        base::generate_mipmaps(_pixels.get(), _size, _mipmaps_count);
    }

    texture_mipmap_span<T> mipmaps(this auto& it, bool skip_whole_texture = false) {
        return {it.data(), it.size(), skip_whole_texture ? 1 : 0, it.mipmaps_count()};
    }

    texture<T> rendered_with_mipmaps() const {
        if (_mipmaps_count == 0)
            return *this;

        auto mipmap_size = clamp_mipmap_size(_size / 2);
        auto new_size = _size + vec2u{mipmap_size.x(), 0};

        texture<T> result{new_size};

        for (auto&& [dst, src] : core::zip{result.crop({0, 0}, _size).pixels(), this->pixels()})
            dst = src;

        uint y = 0;
        auto mipmaps_region = result.crop({_size.x(), 0}, {mipmap_size.x(), _size.y()});

        for (auto&& mipmap : mipmaps(true)) {
            for (auto&& [dst, src] : core::zip(mipmaps_region.crop({0, y}, mipmap_size).pixels(), mipmap.pixels()))
                dst = src;
            y += mipmap_size.y();
            mipmap_size = clamp_mipmap_size(mipmap_size / 2);
        }

        return result;
    }

private:
    texture(malloc_box<T[]>&& pixels, const vec2u& size, size_t mipmaps_count, size_t overal_len):
        _pixels(core::mov(pixels)),
        _size(vec2<u32>(size)),
        _mipmaps_count(u32(mipmaps_count)),
        _overal_len(u32(overal_len)) {}

    vec2<u32>       _size;
    u32             _mipmaps_count;
    u32             _overal_len;
    malloc_box<T[]> _pixels;
};

using texture_g    = texture<clr8_g>;
using texture_ga   = texture<clr8_ga>;
using texture_rgb  = texture<clr8_rgb>;
using texture_rgba = texture<clr8_rgba>;

using texture_hdr_g    = texture<clrf_g>;
using texture_hdr_ga   = texture<clrf_ga>;
using texture_hdr_rgb  = texture<clrf_rgb>;
using texture_hdr_rgba = texture<clrf_rgba>;
} // namespace grx
