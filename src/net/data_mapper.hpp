#pragma once

#include <charconv>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <string>
#include <type_traits>

namespace core::net {
class data_map_error : public std::exception {};

class invalid_address_alignment : public data_map_error {
public:
    invalid_address_alignment(uintptr_t address, size_t alignment) {
        char num[32];
        auto end = std::to_chars(num, num + sizeof(num), address, 16).ptr;
        msg += "invalid address 0x";
        msg += std::string_view(num, end);
        msg += " alignment: ";
        msg += std::to_string(alignment);
        msg += " required";
    }

    [[nodiscard]]
    const char* what() const noexcept override {
        return msg.data();
    }

private:
    std::string msg;
};

class invalid_data_map_size : public data_map_error {
public:
    invalid_data_map_size(size_t actual, size_t needed) {
        msg += "invalid size of map range: actual(";
        msg += std::to_string(actual);
        msg += ") < min(";
        msg += std::to_string(needed);
        msg += ")";
    }

    [[nodiscard]]
    const char* what() const noexcept override {
        return msg.data();
    }

private:
    std::string msg;
};

template <typename T> requires std::is_trivial_v<T>
T& map_as(void* data, size_t size) {
    auto addr = (uintptr_t)data;
    if (addr & alignof(T))
        throw invalid_address_alignment(addr, alignof(T));

    if (sizeof(T) > size)
        throw invalid_data_map_size(size, sizeof(T));

    return *(T*)data;
}

template <typename T> requires std::is_trivial_v<T>
const T& map_as(const void* data, size_t size) {
    return map_as<T>((void*)data, size);
}

namespace details {
    template <typename T>
    struct _sizeof {
        static inline constexpr size_t value = sizeof(T);
    };

    template <>
    struct _sizeof<void> {
        static inline constexpr size_t value = 1;
    };
}

template <typename T, typename U>
    requires requires(U& v) {
        { v.size() } -> std::convertible_to<size_t>;
        { v.data() } -> std::convertible_to<void*>;
    }
T& map_as(U& data) {
    constexpr auto szof = details::_sizeof<std::remove_const_t<std::remove_pointer_t<decltype(data.data())>>>::value;
    return map_as<T>(data.data(), data.size() * szof);
}

template <typename T, typename U>
    requires requires(const U& v) {
        { v.size() } -> std::convertible_to<size_t>;
        { v.data() } -> std::convertible_to<const void*>;
    }
const T& map_as(const U& data) {
    constexpr auto szof = details::_sizeof<std::remove_const_t<std::remove_pointer_t<decltype(data.data())>>>::value;
    return map_as<T>(data.data(), data.size() * szof);
}

template <size_t S>
class as_flat {
public:
    template <typename T>
    as_flat(T&& data): p((const char*)&data) {}

    [[nodiscard]]
    const char* data() const noexcept {
        return p;
    }

    [[nodiscard]]
    constexpr size_t size() const noexcept {
        return S;
    }

private:
    const char* p;
};

template <typename T>
as_flat(T&&) -> as_flat<sizeof(T)>;

} // namespace core::net
