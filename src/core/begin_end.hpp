#pragma once

#include "basic_types.hpp"

namespace core {
template <typename T>
requires requires(const T&& c) {
    {c.begin()};
}
constexpr auto begin(T&& c) {
    return c.begin();
}

template <typename T, size_t S>
constexpr const T* begin(const T(&c)[S]) {
    return c;
}

template <typename T, size_t S>
constexpr T* begin(T(&c)[S]) {
    return c;
}

template <typename T>
requires requires(T&& c) {
    {c.end()};
}
constexpr auto end(T&& c) {
    return c.end();
}

template <typename T, size_t S>
constexpr T* end(T (&c)[S]) {
    return c + S;
}

template <typename T, size_t S>
constexpr const T* end(const T (&c)[S]) {
    return c + S;
}
} // namespace core
