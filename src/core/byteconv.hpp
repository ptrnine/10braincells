#pragma once

/* XXX: maybe remove this?
 * lol
 */

#include <vector>

#include <core/as_tuple.cg.hpp>
#include <core/byteview.hpp>
#include <core/concepts/trivial_span_like.hpp>
#include <core/ranges/range.hpp>
#include <core/traits/is_class.hpp>

namespace core {

using byteconv_size_type = u32;

template <typename T>
constexpr auto to_bytes(const T& value, auto&& out) requires trivial<T> {
    //__builtin_printf("PF: %s\n", __PRETTY_FUNCTION__);
    auto pos = out.size();
    out.resize(pos + sizeof(value));
    __builtin_memcpy(out.data() + pos, &value, sizeof(value));
}

template <typename T>
constexpr auto to_bytes(const T& value, auto&& out) requires (!trivial<T>) && trivial_span_like<T> {
    //__builtin_printf("PF: %s\n", __PRETTY_FUNCTION__);
    to_bytes(byteconv_size_type(value.size()), out);

    auto pos = out.size();
    auto sz  = value.size() * sizeof(*value.data());
    out.resize(pos + sz);
    __builtin_memcpy(out.data() + pos, value.data(), sz);
}

template <typename T>
constexpr auto to_bytes(const T& value, auto&& out) requires(!trivial<T> && !trivial_span_like<T> && !input_range<T> && is_class<T>);

template <typename T>
constexpr auto to_bytes(const T& value, auto&& out) requires(!trivial<T> && !trivial_span_like<T> && input_range<T>) {
    //__builtin_printf("PF: %s\n", __PRETTY_FUNCTION__);

    auto size_pos = out.size();
    to_bytes(byteconv_size_type(0), out);

    byteconv_size_type count = 0;
    for (const auto& v : value) {
        to_bytes(v, out);
        ++count;
    }

    __builtin_memcpy(out.data() + size_pos, &count, sizeof(count));
}

template <typename T>
constexpr auto to_bytes(const T& value, auto&& out) requires(!trivial<T> && !trivial_span_like<T> && !input_range<T> && is_class<T>) {
    //__builtin_printf("PF: %s\n", __PRETTY_FUNCTION__);
    as_tuple_ref(value).foreach ([&](const auto& value) {
        to_bytes(value, out);
    });
}

template <typename T, typename ByteT = char> requires (sizeof(ByteT) == 1)
constexpr auto to_bytes(const T& value) requires trivial<T> {
    return std::span{(const ByteT*)&value, sizeof(T)};
}

template <typename T, typename ByteT = char> requires (sizeof(ByteT) == 1)
constexpr auto to_bytes(const T& value) requires (!trivial<T>) {
    std::vector<ByteT> result;
    to_bytes(value, result);
    return result;
}

class from_bytes_out_of_range : public std::exception {
public:
    from_bytes_out_of_range(size_t offset, size_t size, size_t available) {
        msg = "cannot read " + std::to_string(size) + " bytes at offset " + std::to_string(offset) + " because only " +
              std::to_string(available) + " bytes available in the source buffer";
    }

    const char* what() const noexcept override {
        return msg.data();
    }

private:
    std::string msg;
};

template <typename T, typename ByteT = const char> requires (sizeof(ByteT) == 1) && trivial<T>
constexpr size_t from_bytes(std::span<ByteT> bytes, T& value, size_t debug_offset = 0, size_t debug_size = 0) {
    if (bytes.size() < sizeof(T))
        throw from_bytes_out_of_range(debug_offset, sizeof(T), debug_size ? debug_offset : bytes.size());
    __builtin_memcpy(&value, bytes.data(), sizeof(T));
    return sizeof(T);
}

namespace details {
    template <typename T>
    concept resize_and_memcpyable = trivial_span_like<T> && requires (T& container) {
        {container.resize(size_t(0))};
    };

    template <typename T>
    concept emplacable = requires(T& container) {
        {container.begin()} -> core::input_iterator;
        {container.emplace(*container.begin())};
    };

    template <typename T>
    concept emplace_backable = requires(T& container) {
        {container.back()};
        {container.emplace_back()};
    };
} // namespace details

template <typename T, typename ByteT = const char> requires (sizeof(ByteT) == 1) && (!trivial<T>) && details::resize_and_memcpyable<T>
constexpr size_t from_bytes(std::span<ByteT> bytes, T& value, size_t debug_offset = 0, size_t debug_size = 0) {
    byteconv_size_type size;
    auto offset = from_bytes(bytes, size, debug_offset, debug_size);
    bytes = bytes.subspan(offset);
    debug_offset += offset;
    size_t total_read = offset;

    value.resize(size);
    auto bs_size = size * sizeof(*value.data());
    if (bytes.size() < bs_size)
        throw from_bytes_out_of_range(debug_offset, size * sizeof(*value.data()), debug_size ? debug_offset : bytes.size());
    __builtin_memcpy(value.data(), bytes.data(), bs_size);
    total_read += bs_size;

    return total_read;
}

template <typename T, typename ByteT = const char>
    requires (sizeof(ByteT) == 1) && (!trivial<T>) && (!details::resize_and_memcpyable<T>) && details::emplacable<T>
constexpr size_t from_bytes(std::span<ByteT> bytes, T& value, size_t debug_offset = 0, size_t debug_size = 0) {
    byteconv_size_type size;
    auto offset = from_bytes(bytes, size, debug_offset, debug_size);
    bytes = bytes.subspan(offset);
    debug_offset += offset;
    size_t total_read = offset;

    for (byteconv_size_type i = 0; i < size; ++i) {
        remove_const_ref<decltype(*value.begin())> v;
        offset = from_bytes(bytes, v, debug_offset, debug_size);
        value.emplace(mov(v));
        bytes = bytes.subspan(offset);
        debug_offset += offset;
        total_read += offset;
    }

    return total_read;
}

template <typename T, typename ByteT = const char>
    requires (sizeof(ByteT) == 1) && (!trivial<T>) && (!details::resize_and_memcpyable<T>) && (!details::emplacable<T>) &&
             details::emplace_backable<T>
constexpr size_t from_bytes(std::span<ByteT> bytes, T& value, size_t debug_offset = 0, size_t debug_size = 0) {
    byteconv_size_type size;
    auto offset = from_bytes(bytes, size, debug_offset, debug_size);
    bytes = bytes.subspan(offset);
    debug_offset += offset;
    size_t total_read = offset;

    for (byteconv_size_type i = 0; i < size; ++i) {
        value.emplace_back();
        offset = from_bytes(bytes, value.back(), debug_offset, debug_size);
        bytes = bytes.subspan(offset);
        debug_offset += offset;
        total_read += offset;
    }

    return total_read;
}

template <typename T, typename ByteT = const char>
    requires (sizeof(ByteT) == 1) && (!trivial<T>) && (!details::resize_and_memcpyable<T>) && (!details::emplacable<T>) &&
             (!details::emplace_backable<T>) && is_class<T>
constexpr size_t from_bytes(std::span<ByteT> bytes, T& value, size_t debug_offset = 0, size_t debug_size = 0) {
    size_t total_read = 0;
    as_tuple_ref(value).foreach([&](auto& value) {
        auto offset = from_bytes(bytes, value, debug_offset, debug_size);
        debug_offset += offset;
        total_read += offset;
        bytes = bytes.subspan(offset);
    });
    return total_read;
}

void test() {
}

template <typename T, typename ByteT = const char> requires (sizeof(ByteT) == 1)
constexpr decltype(auto) from_bytes(std::span<ByteT> bytes) requires trivial<T> {
    if (bytes.size() < sizeof(T))
        throw from_bytes_out_of_range(0, sizeof(T), bytes.size());
    return from_byteview<T>(bytes.data(), bytes.size());
}

template <typename T, typename ByteT = const char> requires (sizeof(ByteT) == 1)
constexpr decltype(auto) from_bytes(std::span<ByteT> bytes) requires (!trivial<T>) {
    T result;
    from_bytes(bytes, result, 0, bytes.size());
    return result;
}

namespace details {
    template <typename>
    struct not_a_span {
        static inline constexpr bool value = true;
    };

    template <typename T, size_t Extent>
    struct not_a_span<std::span<T, Extent>> {
        static inline constexpr bool value = false;
    };
}

template <typename T, typename U>
constexpr decltype(auto) from_bytes(const U& bytes) requires details::not_a_span<T>::value {
    return from_bytes<T>(std::span{bytes});
}
} // namespace core
