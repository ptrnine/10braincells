#pragma once

#include <charconv>
#include <string>

#include <core/basic_types.hpp>
#include <core/concepts/convertible_to.hpp>
#include <core/concepts/trivial.hpp>
#include <core/exception.hpp>
#include <core/traits/remove_const.hpp>
#include <core/traits/remove_ptr.hpp>

namespace core
{
class byteview_error : public exception {};

class invalid_byteview_alignment : public byteview_error {
public:
    invalid_byteview_alignment(uptr address, size_t alignment) {
        char num[32];
        auto end = std::to_chars(num, num + sizeof(num), address, 16).ptr;
        msg += "invalid address 0x";
        msg += std::string_view(num, end);
        msg += " (must be aligned by ";
        msg += std::to_string(alignment);
        msg += ")";
    }

    [[nodiscard]]
    const char* what() const noexcept override {
        return msg.data();
    }

private:
    std::string msg;
};

class invalid_byteview_size : public byteview_error {
public:
    invalid_byteview_size(size_t actual, size_t needed) {
        msg += "invalid size of view range: actual(";
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

template <trivial T>
constexpr T& from_byteview(void* data, size_t size) {
    auto addr = (uintptr_t)data;
    if (addr % alignof(T))
        throw invalid_byteview_alignment(addr, alignof(T));

    if (sizeof(T) > size)
        throw invalid_byteview_size(size, sizeof(T));

    return *(T*)data;
}

template <trivial T>
constexpr const T& from_byteview(const void* data, size_t size) {
    return from_byteview<T>((void*)data, size);
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
        { v.size() } -> convertible_to<size_t>;
        { v.data() } -> convertible_to<void*>;
    }
constexpr T& from_byteview(U& data) {
    constexpr auto szof = details::_sizeof<remove_const<remove_ptr<decltype(data.data())>>>::value;
    return from_byteview<T>(data.data(), data.size() * szof);
}

template <typename T, typename U>
    requires requires(const U& v) {
        { v.size() } -> convertible_to<size_t>;
        { v.data() } -> convertible_to<const void*>;
    }
constexpr const T& from_byteview(const U& data) {
    constexpr auto szof = details::_sizeof<remove_const<remove_ptr<decltype(data.data())>>>::value;
    return from_byteview<T>(data.data(), data.size() * szof);
}

template <size_t S>
class byteview {
public:
    template <typename T>
    constexpr byteview(T&& data): p((const char*)&data) {}

    [[nodiscard]]
    constexpr const char* data() const noexcept {
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
byteview(T&&) -> byteview<sizeof(T)>;
} // namespace core
