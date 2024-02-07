#pragma once

#include "basic_types.hpp"
#include "concepts/inheritance.hpp"
#include "concepts/integral.hpp"

namespace core {
template <typename T>
concept ebo_super = empty_class<T> && !final_class<T>;

template <auto idx, typename T>
struct ebo {
    T data;
};

template <auto idx, ebo_super T>
struct ebo<idx, T> : T{};

template <auto idx, typename T> requires (!integral<decltype(idx)>)
constexpr T& get(ebo<idx, T>& e) noexcept {
    return e.data;
}

template <auto idx, typename T> requires (!integral<decltype(idx)>)
constexpr const T& get(const ebo<idx, T>& e) noexcept {
    return e.data;
}

template <auto idx, typename T> requires (!integral<decltype(idx)>)
constexpr T&& get(ebo<idx, T>&& e) noexcept {
    return static_cast<T&&>(e.data);
}

template <auto idx, ebo_super T> requires (!integral<decltype(idx)>)
constexpr T& get(ebo<idx, T>& e) noexcept {
    return e;
}

template <auto idx, ebo_super T> requires (!integral<decltype(idx)>)
constexpr const T& get(const ebo<idx, T>& e) noexcept {
    return e;
}

template <auto idx, ebo_super T> requires (!integral<decltype(idx)>)
constexpr T&& get(ebo<idx, T>&& e) noexcept {
    return static_cast<T&&>(e);
}

/* Workaround for gcc bug with structured bindings */
template <size_t idx, typename T>
constexpr T& get(ebo<idx, T>& e) noexcept {
    return e.data;
}

template <size_t idx, typename T>
constexpr const T& get(const ebo<idx, T>& e) noexcept {
    return e.data;
}

template <size_t idx, typename T>
constexpr T&& get(ebo<idx, T>&& e) noexcept {
    return static_cast<T&&>(e.data);
}

template <size_t idx, ebo_super T>
constexpr T& get(ebo<idx, T>& e) noexcept {
    return e;
}

template <size_t idx, ebo_super T>
constexpr const T& get(const ebo<idx, T>& e) noexcept {
    return e;
}

template <size_t idx, ebo_super T>
constexpr T&& get(ebo<idx, T>&& e) noexcept {
    return static_cast<T&&>(e);
}
} // namespace core
