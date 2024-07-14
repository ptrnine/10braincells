#pragma once

#ifdef DISABLE_SINGLE_OBJECT_BUILD
#include <stb_image.h>
#include <stb_image_resize2.h>
#include <stb_image_write.h>
#else
#include <grx/stb_sources.hpp>
#endif

#include <core/bit.hpp>
#include <core/concepts/number.hpp>
#include <core/finalizer.hpp>
#include <core/ranges/zip.hpp>
#include <core/tuple.hpp>
#include <grx/color.hpp>
#include <util/log.hpp>
#include <util/print.hpp>
#include <util/ranges/dimensional_seq.hpp>

namespace grx
{
using core::unsigned_integral;
using core::same_as;
using core::floating_point;

constexpr u32  MIPMAPS_COUNT   = 8;
constexpr auto MIPMAP_MIN_SIZE = vec{2U, 2U};

/**
 * @brief Provides access to the pixel data
 *
 * @tparam T - the type of the color channel value
 * @tparam NPP - channels count
 * @tparam Const - const flag
 */
template <color_component T, size_t NPP, bool Const = false>
class texture_pixel_view {
public:
    texture_pixel_view(T* data): data(data) {}

    /**
     * @brief Sets pixel data from the color
     *
     * @tparam TT - the type of the color channel value
     * @param color - the color
     *
     * @return *this
     */
    texture_pixel_view& operator=(const vec<T, NPP>& color) requires (!Const) {
        memcpy(data, color.v.data(), NPP * sizeof(T));
        return *this;
    }

    /**
     * @brief Gets color from the pixel
     *
     * @return the color
     */
    vec<T, NPP> get() const {
        vec<T, NPP> color;
        memcpy(color.v.data(), data, NPP * sizeof(T));
        return color;
    }

    void print(std::ostream& os) const {
        if constexpr (core::integral<T>) {
            std::string str = "#";
            auto color = static_cast<vec<u8, NPP>>(get());
            for (auto c : color.v) {
                str.push_back("0123456789abcdef"[c >> 4]);
                str.push_back("0123456789abcdef"[c & 0x0f]);
            }
            os << str;
        }
        else {
            os << util::format("{}", get());
        }
    }

private:
    T* data;
};

template <color_component T, size_t NPP, bool Const = false>
class texture_span_iterator {
public:
    using iterator_category = std::random_access_iterator_tag;
    using difference_type   = diff_t;

    texture_span_iterator() = default;
    texture_span_iterator(T* pointer): ptr(pointer) {}
    texture_span_iterator(const texture_span_iterator<T, NPP, true>& it) noexcept: ptr(it.ptr) {}

    texture_span_iterator& operator++() noexcept {
        ptr += NPP;
        return *this;
    }

    texture_span_iterator& operator--() noexcept {
        ptr -= NPP;
        return *this;
    }

    texture_span_iterator operator++(int) noexcept {
        auto res = *this;
        ptr += NPP;
        return res;
    }

    texture_span_iterator operator--(int) noexcept {
        auto res = *this;
        ptr -= NPP;
        return res;
    }

    texture_span_iterator& operator+=(difference_type n) noexcept {
        ptr += n * NPP;
        return *this;
    }

    texture_span_iterator& operator-=(difference_type n) noexcept {
        ptr -= n * NPP;
        return *this;
    }

    texture_span_iterator operator+(difference_type n) const noexcept {
        auto res = *this;
        res += n;
        return res;
    }

    texture_span_iterator operator-(difference_type n) const noexcept {
        auto res = *this;
        res -= n;
        return res;
    }

    difference_type operator-(const texture_span_iterator& it) const noexcept {
        return (ptr - it.ptr) / NPP;
    }

    auto operator<=>(const texture_span_iterator& it) const noexcept = default;

    texture_pixel_view<T, NPP, Const> operator*() const noexcept {
        return {ptr};
    }

    texture_pixel_view<T, NPP, Const> operator[](difference_type n) const noexcept {
        return *(*this + n);
    }


private:
    T* ptr = nullptr;
};

/**
 * @brief Provides access to the row of pixels
 *
 * @tparam T - the type of the color channel value
 * @tparam NPP - channels count
 * @tparam Const - const flag
 */
template <color_component T, size_t NPP, bool Const = false>
class texture_row_view {
public:
    texture_pixel_view<T, NPP, Const> operator[](size_t pos) const noexcept {
        return {data + pos * NPP};
    }

    texture_pixel_view<T, NPP, Const> at(size_t pos) const {
        if (pos >= width)
            throw std::out_of_range("Out of range");

        return {data + pos * NPP};
    }

    texture_span_iterator<T, NPP, Const> begin() const noexcept {
        return {data};
    }

    texture_span_iterator<T, NPP, Const> end() const noexcept {
        return {data + width * NPP};
    }

private:
    template <color_component, size_t>
    friend class texture;

    template <color_component, size_t, bool>
    friend class texture_span;

    texture_row_view(T* idata, uint iwidth) noexcept: data(idata), width(iwidth) {}

private:
    T*   data;
    uint width;
};

template <color_component T, size_t NPP, bool Const>
class texture_span {
public:
    texture_span(T* idata, const vec2u& isize): ptr(idata), sz(isize) {}

    const vec2u& size() const noexcept {
        return sz;
    }

    size_t pixels_count() const noexcept {
        return sz.x() * sz.y() * NPP;
    }

    T* data() const noexcept {
        return ptr;
    }

    texture_span_iterator<T, NPP, Const> begin() const noexcept {
        return {ptr};
    }

    texture_span_iterator<T, NPP, Const> end() const noexcept {
        return {ptr + sz.x() * sz.y() * NPP};
    }

    texture_row_view<T, NPP, Const> operator[](size_t row_num) const noexcept {
        return {ptr + row_num * sz.x() * NPP, sz.x()};
    }

    template <typename IdxT>
    texture_pixel_view<T, NPP, Const> operator[](vec<IdxT, 2> pos) const noexcept {
        return (*this)[pos.y()][pos.x()];
    }

    void flip_horizontal() {
        auto half_size = vec2u{sz.x() / 2, sz.y()};
        for (auto [x, y] : util::dimensional_seq(half_size)) {
            auto tmp_color             = (*this)[y][x].get();
            (*this)[y][x]              = (*this)[y][sz.x() - x - 1].get();
            (*this)[y][sz.x() - x - 1] = tmp_color;
        }
    }

    void flip_vertical() {
        auto half_size = vec2u{sz.x(), sz.y() / 2};
        for (auto [x, y] : util::dimensional_seq(half_size)) {
            auto tmp_color             = (*this)[y][x].get();
            (*this)[y][x]              = (*this)[sz.y() - y - 1][x].get();
            (*this)[sz.y() - y - 1][x] = tmp_color;
        }
    }

private:
    T*    ptr;
    vec2u sz;
};

template <color_component T, size_t NPP, bool Const>
class texture_level_view;

template <color_component T, size_t NPP, bool Const>
class texture_level_iterator {
public:
    friend class texture_level_view<T, NPP, Const>;
    using iterator_category = std::bidirectional_iterator_tag;

    static core::tuple<vec2u, size_t> skip_compcount(vec2u size, uint mipmap) {
        size_t skip = 0;
        for ([[maybe_unused]] uint i = 0; i < mipmap; ++i) {
            skip += size.x() * size.y() * NPP;
            size /= 2U;
            if (size.x() < MIPMAP_MIN_SIZE.x())
                size.x() = MIPMAP_MIN_SIZE.x();
            if (size.y() < MIPMAP_MIN_SIZE.y())
                size.y() = MIPMAP_MIN_SIZE.y();
        }
        return {size, skip};
    }

    texture_level_iterator& operator++() noexcept {
        if (mipmap < max_mipmaps)
            ++mipmap;
        return *this;
    }

    texture_level_iterator& operator--() noexcept {
        if (mipmap != 0)
            --mipmap;
        return *this;
    }

    texture_level_iterator operator++(int) noexcept {
        auto res = *this;
        ++(*this);
        return res;
    }

    texture_level_iterator operator--(int) noexcept {
        auto res = *this;
        --(*this);
        return res;
    }

    bool operator==(const texture_level_iterator& it) const noexcept {
        return mipmap == it.mipmap;
    }

    bool operator!=(const texture_level_iterator& it) const noexcept {
        return !(*this == it);
    }

    texture_span<T, NPP, Const> operator*() const noexcept {
        auto [sz, skip] = skip_compcount(size, mipmap);
        return {data + skip, sz};
    }

private:
    texture_level_iterator(const vec2u& isize, uint imipmap_count, T* idata, uint imipmap = 0):
        size(isize), max_mipmaps(imipmap_count + 1), data(idata), mipmap(imipmap) {}

    vec2u size;
    uint  max_mipmaps;
    T*    data;
    uint  mipmap = 0;
};

template <color_component T, size_t NPP, bool Const>
class texture_level_view {
public:
    texture_level_view(T* data, const vec2u& size, uint mipmaps_count):
        b(size, mipmaps_count, data), e(size, mipmaps_count, data, mipmaps_count + 1) {}

    auto begin() const noexcept {
        return b;
    }

    auto end() const noexcept {
        return e;
    }

private:
    texture_level_iterator<T, NPP, Const> b;
    texture_level_iterator<T, NPP, Const> e;
};

class stbi_write_image_error : public core::exception {
public:
    stbi_write_image_error(std::string message): msg(core::mov(message)) {}

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

enum class image_format { jpg, png, tga, bmp };

namespace details
{
    void stbi_write_callback(void* byte_vector, void* data, int size) {
        auto vec      = static_cast<std::vector<byte>*>(byte_vector);
        auto old_size = vec->size();
        vec->resize(old_size + size_t(size));
        std::memcpy(vec->data() + old_size, data, static_cast<size_t>(size)); // NOLINT
    }

    template <typename Derived, typename T, size_t NPP>
    class texture_u8 {
    public:
    };

    template <typename Derived, size_t NPP>
    class texture_u8<Derived, u8, NPP> {
    public:
        std::vector<core::byte> to_png(this const auto& it) {
            std::vector<byte> result;
            auto [w, h] = vec2i{it.size()};
            auto ok =
                stbi_write_png_to_func(details::stbi_write_callback, &result, w, h, int(NPP), it.data(), w * int(NPP));
            if (!ok)
                throw stbi_write_image_error("Cannot convert texture to png image");
            return result;
        }

        std::vector<core::byte> to_tga(this const auto& it) {
            std::vector<byte> result;
            auto [w, h] = vec2i{it.size()};
            auto ok     = stbi_write_tga_to_func(details::stbi_write_callback, &result, w, h, int(NPP), it.data());
            if (!ok)
                throw stbi_write_image_error("Cannot convert texture to tga image");
            return result;
        }

        std::vector<core::byte> to_bmp(this const auto& it) {
            std::vector<byte> result;
            auto [w, h] = vec2i{it.size()};
            auto ok     = stbi_write_bmp_to_func(details::stbi_write_callback, &result, w, h, int(NPP), it.data());
            if (!ok)
                throw stbi_write_image_error("Cannot convert texture to tga image");
            return result;
        }

        std::vector<core::byte> to_jpg(this const auto& it, int quality = 95) {
            std::vector<byte> result;
            auto [w, h] = vec2i{it.size()};
            auto ok = stbi_write_jpg_to_func(details::stbi_write_callback, &result, w, h, int(NPP), it.data(), quality);
            if (!ok)
                throw stbi_write_image_error("Cannot convert texture to tga image");
            return result;
        }

        static Derived from_image(std::span<const core::byte> image) {
            constexpr int type = [] {
                switch (NPP) {
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

            int  w, h, comp;
            auto img =
                stbi_load_from_memory((const uint8_t*)(image.data()), int(image.size()), &w, &h, &comp, type); // NOLINT
            core::finalizer guard{[&] {
                if (img)
                    stbi_image_free(img);
            }};

            if (!img)
                throw stbi_read_image_error("Cannot convert image to texture");

            if (comp != NPP)
                throw stbi_read_image_error(util::format("Invalid channels count: required={} actual={}", NPP, comp));

            return {img, vec2u{uint(w), uint(h)}};
        }
    };
} // namespace details

template <color_component T, size_t NPP>
class texture : public details::texture_u8<texture<T, NPP>, T, NPP> {
public:
    static size_t calc_pixels_count(const vec2u& size, u32 mipmaps) {
        size_t pixels_count = size.x() * size.y() * NPP;
        auto   mipmap_size  = size / 2U;

        for (u32 i = 0; i < mipmaps; ++i) {
            pixels_count += mipmap_size.x() * mipmap_size.y() * NPP;
            mipmap_size /= 2U;
            if (mipmap_size.x() < MIPMAP_MIN_SIZE.x())
                mipmap_size.x() = MIPMAP_MIN_SIZE.x();
            if (mipmap_size.y() == MIPMAP_MIN_SIZE.y())
                mipmap_size.y() = MIPMAP_MIN_SIZE.y();
        }

        return pixels_count;
    }

    static constexpr stbir_pixel_layout stbi_pixel_layout = []{
        switch (NPP) {
        case 1:
            return STBIR_1CHANNEL;
        case 2:
            return STBIR_2CHANNEL;
        case 3:
            return STBIR_RGB;
        case 4:
            return STBIR_RGBA;
        }
    }();

    texture(T* raw_data, const vec2u& size, u32 mipmaps = 0): _size(size), _mipmaps_count(mipmaps) {
        if (mipmaps && (!core::is_power_of_two(size.x()) || !core::is_power_of_two(size.y()))) {
            auto newsize = vec{core::next_power_of_two(_size.x()), core::next_power_of_two(_size.y())};
            util::glog().warn("sizes for mipmaping must be power of two: change size from {} to {}", _size, newsize);
            _size = newsize;
        }

        _pixels_count = calc_pixels_count(_size, _mipmaps_count);
        _data         = std::make_unique<T[]>(_pixels_count);
        memcpy(_data.get(), raw_data, _pixels_count * sizeof(T));
    }

    texture(const vec2u& size, u32 mipmaps = 0): _size(size), _mipmaps_count(mipmaps) {
        if (mipmaps && (!core::is_power_of_two(size.x()) || !core::is_power_of_two(size.y()))) {
            auto newsize = vec{core::next_power_of_two(_size.x()), core::next_power_of_two(_size.y())};
            util::glog().warn("sizes for mipmaping must be power of two: change size from {} to {}", _size, newsize);
            _size = newsize;
        }

        _pixels_count = calc_pixels_count(_size, _mipmaps_count);
        _data         = std::make_unique<T[]>(_pixels_count);
    }

    texture(const texture& map): texture(map._data.get(), map._size, map._mipmaps_count) {}

    texture& operator=(const texture& map) {
        _size         = map._size;
        _pixels_count = map._pixels_count;
        _mipmaps_count      = map._mipmaps_count;
        _data         = std::make_unique<T[]>(_pixels_count);
        memcpy(_data.get(), map._data.get(), _pixels_count * sizeof(T));

        return *this;
    }

    texture(texture&& map) noexcept:
        _size(map._size), _data(core::mov(map._data)), _pixels_count(map._pixels_count), _mipmaps_count(map._mipmaps_count) {
        map._data = nullptr;
    }

    texture& operator=(texture&& map) noexcept {
        _size         = map._size;
        _data         = core::mov(map._data);
        _mipmaps_count      = map._mipmaps_count;
        _pixels_count = map._pixels_count;
        map._data     = nullptr;

        return *this;
    }

    ~texture() noexcept = default;

#if 0
    static texture from_bytes(core::span<core::byte> bytes) {
        if constexpr (std::endian::native == std::endian::big)
            pe_throw std::runtime_error("Implement me for big endian");

        using namespace core;

        deserializer_view ds{bytes};
        ds.read_get<array<char, 4>>();

        auto channels = ds.read_get<u32>();
        PeRelRequireF(channels == NPP, "Invalid channels count: gets {} but {} required", channels, NPP);

        auto size = ds.read_get<vec<u32, 2>>();
        auto mipmaps = ds.read_get<u32>();

        if (mipmaps && (!core::is_power_of_two(size.x()) || !core::is_power_of_two(size.y())))
            pe_throw std::runtime_error("Sizes must be power of two");

        ds.read_get<u64>(); /* Skip compressed size */

        auto pixels_count      = calc_pixels_count(size, mipmaps);
        auto data           = std::make_unique<T[]>(pixels_count);
        auto img_compressed = bytes.subspan(28);
        auto img_data       = util::decompress_block(img_compressed, pixels_count * sizeof(T));

        PeRelRequireF(pixels_count == static_cast<size_t>(img_data.size()),
                      "Invalid image size: gets {} but {} required",
                      img_data.size(),
                      pixels_count);
        memcpy(data.get(), img_data.data(), pixels_count * sizeof(T));

        texture result;
        result._size = size;
        result._data = core::mov(data);
        result._mipmaps_count = mipmaps;
        result._pixels_count = pixels_count;

        return result;
    }

    core::vector<core::byte> to_bytes() const {
        if constexpr (std::endian::native == std::endian::big)
            pe_throw std::runtime_error("Implement me for big endian");

        using namespace core;

        serializer s;
        s.write(array{'P', 'E', 'T', 'X'});
        s.write(static_cast<u32>(NPP));
        s.write(static_cast<vec<u32, 2>>(_size));
        s.write(_mipmaps_count);
        s.write(util::compress(span{reinterpret_cast<core::byte*>(_data.get()), // NOLINT
                                    static_cast<ssize_t>(_pixels_count)},
                               util::COMPRESS_FAST));

        return s.detach_data();
    }
#endif

    void gen_mipmaps() {
        if (!core::is_power_of_two(_size.x()) || !core::is_power_of_two(_size.y()))
            throw std::runtime_error("Sizes must be power of two");

        auto pixels_count     = calc_pixels_count(_size, MIPMAPS_COUNT);
        auto newdata          = std::make_unique<T[]>(pixels_count);
        auto ptr              = newdata.get();
        auto src_pixels_count = _size.x() * _size.y() * NPP;
        std::memcpy(ptr, _data.get(), src_pixels_count * sizeof(T));

        auto   mipmap_size      = _size;
        size_t src_displacement = 0;

        for (u32 i = 0; i < MIPMAPS_COUNT; ++i) {
            auto new_mipmap_size = mipmap_size / 2U;
            if (mipmap_size.x() < MIPMAP_MIN_SIZE.x())
                mipmap_size.x() = MIPMAP_MIN_SIZE.x();
            if (mipmap_size.y() == MIPMAP_MIN_SIZE.y())
                mipmap_size.y() = MIPMAP_MIN_SIZE.y();

            if constexpr (floating_point<T>)
                stbir_resize_float_linear(ptr + src_displacement,
                                          int(mipmap_size.x()),
                                          int(mipmap_size.y()),
                                          0,
                                          ptr + src_displacement + src_pixels_count,
                                          int(new_mipmap_size.x()),
                                          int(new_mipmap_size.y()),
                                          0,
                                          stbi_pixel_layout);
            else
                stbir_resize_uint8_linear(ptr + src_displacement,
                                          int(mipmap_size.x()),
                                          int(mipmap_size.y()),
                                          0,
                                          ptr + src_displacement + src_pixels_count,
                                          int(new_mipmap_size.x()),
                                          int(new_mipmap_size.y()),
                                          0,
                                          stbi_pixel_layout);

            auto dst_pixels_count = new_mipmap_size.x() * new_mipmap_size.y() * NPP;

            src_displacement += src_pixels_count;
            src_pixels_count = dst_pixels_count;
            mipmap_size      = new_mipmap_size;
        }

        _data         = core::mov(newdata);
        _mipmaps_count      = MIPMAPS_COUNT;
        _pixels_count = pixels_count;
    }

    texture_level_view<T, NPP, false> mipmap_view() {
        return texture_level_view<T, NPP, false>(_data.get(), _size, _mipmaps_count);
    }

    texture_level_view<T, NPP, true> mipmap_view() const {
        return texture_level_view<T, NPP, true>(_data.get(), _size, _mipmaps_count);
    }

    texture_span_iterator<T, NPP> begin() noexcept {
        return {_data.get()};
    }

    texture_span_iterator<T, NPP> end() noexcept {
        return {_data.get() + _size.x() * _size.y() * NPP};
    }

    texture_span_iterator<T, NPP, true> begin() const noexcept {
        return {_data.get()};
    }

    texture_span_iterator<T, NPP, true> end() const noexcept {
        return {_data.get() + _size.x() * _size.y() * NPP};
    }

    texture_row_view<T, NPP, true> operator[](size_t row_num) const noexcept {
        return {_data.get() + row_num * _size.x() * NPP, _size.x()};
    }

    texture_row_view<T, NPP> operator[](size_t row_num) noexcept {
        return {_data.get() + row_num * _size.x() * NPP, _size.x()};
    }

    template <typename IdxT>
    texture_pixel_view<T, NPP, true> operator[](vec<IdxT, 2> pos) const noexcept {
        return (*this)[pos.y()][pos.x()];
    }

    template <typename IdxT>
    texture_pixel_view<T, NPP, false> operator[](vec<IdxT, 2> pos) noexcept {
        return (*this)[pos.y()][pos.x()];
    }

    /*
    template <typename TT>
    explicit operator texture<TT, NPP>() const {
        texture<TT, NPP> result(_size);

        for (auto& [dst, src] : zip_view(result, *this))
            dst = src.get();

        return result;
    }
    */

    const vec2u& size() const noexcept {
        return _size;
    }

    const T* data() const noexcept {
        return _data.get();
    }

    T* data() noexcept {
        return _data.get();
    }

    bool has_mipmaps() const noexcept {
        return _mipmaps_count > 0;
    }

    u32 mipmaps_count() const noexcept {
        return _mipmaps_count;
    }

    size_t pixels_count() const noexcept {
        return _pixels_count;
    }

    texture resized(const vec2u& new_size) const {
        auto output_pixels = std::make_unique<T[]>(new_size.x() * new_size.y() * NPP);

        if constexpr (core::floating_point<T>)
            stbir_resize_float_linear(_data.get(),
                                      int(_size.x()),
                                      int(_size.y()),
                                      0,
                                      output_pixels.get(),
                                      int(new_size.x()),
                                      int(new_size.y()),
                                      0,
                                      stbi_pixel_layout);
        else
            stbir_resize_uint8_linear(_data.get(),
                                      int(_size.x()),
                                      int(_size.y()),
                                      0,
                                      output_pixels.get(),
                                      int(new_size.x()),
                                      int(new_size.y()),
                                      0,
                                      stbi_pixel_layout);

        texture result;
        result._data         = core::mov(output_pixels);
        result._size         = new_size;
        result._pixels_count = new_size.x() * new_size.y() * NPP;

        return result;
    }

    template <bool Enable = std::is_floating_point_v<T>>
    auto to_ldr(float gamma = 1.f, float exposure = 1.f) const
        -> std::enable_if_t<Enable, texture<u8, NPP>> {
        auto result = texture<u8, NPP>(_size, _mipmaps_count);
        for (size_t i = 0; i < _pixels_count; ++i) {
            auto mapped = T(1.0) - std::exp(-_data[i] * exposure);
            result.data()[i] = u8(T(255.0) * std::pow(mapped, T(1.0) / gamma));
        }
        return result;
    }

    template <bool Enable = !std::is_floating_point_v<T>>
    auto to_hdr() const -> std::enable_if_t<Enable, texture<float, NPP>> {
        auto result = texture<float, NPP>(_size, _mipmaps_count);
        for (size_t i = 0; i < _pixels_count; ++i)
            result.data()[i] = float(_data[i]) / 255.f;
        return result;
    }

    void flip_horizontal() {
        for (auto cmap_view : mipmap_view())
            cmap_view.flip_horizontal();
    }

    void flip_vertical() {
        for (auto cmap_view : mipmap_view())
            cmap_view.flip_vertical();
    }

    texture flipped_horizontal() const {
        auto res = *this;
        res.flip_horizontal();
        return res;
    }

    texture flipped_vertical() const {
        auto res = *this;
        res.flip_vertical();
        return res;
    }

    texture rotated_left() const {
        auto res      = texture(_size.yx(), _mipmaps_count);
        auto src_view = mipmap_view();
        auto dst_view = res.mipmap_view();

        for (auto [dst, src] : core::zip_view(dst_view, src_view))
            for (auto [x, y] : util::dimensional_seq(src.size()))
                dst[src.size().x() - x - 1][y] = src[y][x].get();

        return res;
    }

    texture rotated_right() const {
        auto res      = texture(_size.yx(), _mipmaps_count);
        auto src_view = mipmap_view();
        auto dst_view = res.mipmap_view();

        for (auto [dst, src] : core::zip_view(dst_view, src_view))
            for (auto [x, y] : util::dimensional_seq(src.size()))
                dst[x][src.size().y() - y - 1] = src[y][x].get();

        return res;
    }

    texture rotated_180() const {
        texture res      = texture(_size, _mipmaps_count);
        auto    src_view = mipmap_view();
        auto    dst_view = res.mipmap_view();

        for (auto [dst, src] : core::zip_view(dst_view, src_view))
            for (auto [x, y] : util::dimensional_seq(src.size()))
                dst[src.size().y() - y - 1][src.size().x() - x - 1] = src[y][x].get();

        return res;
    }

    texture mirrored_down() const {
        auto new_size = vec2u{_size.x(), _size.y() * 2U};
        auto res      = texture(new_size, _mipmaps_count);
        auto dst_view = res.mipmap_view();
        auto src_view = mipmap_view();

        for (auto [dst, src] : core::zip_view(dst_view, src_view)) {
            auto pixels_count = src.pixels_count();
            std::memcpy(dst.data(), src.data(), pixels_count * sizeof(T));
            for (auto [x, y] : util::dimensional_seq(src.size()))
                dst[y + src.size().y()][x] = src[src.size().y() - y - 1][x].get();
        }

        return res;
    }

private:
    template <color_component, size_t>
    friend class grx_texture;

    texture() noexcept = default;

private:
    vec2u                _size          = {0, 0};
    std::unique_ptr<T[]> _data          = nullptr;
    size_t               _pixels_count  = 0;
    u32                  _mipmaps_count = 0;
};

using texture_rgb = texture<u8, 3>;
using texture_rgba = texture<u8, 4>;

using texture_hdr_rgb = texture<float, 3>;
using texture_hdr_rgba = texture<float, 4>;

} // namespace grx
