#pragma once

#include <core/basic_types.hpp>
#include <core/compact_hashes.hpp>
#include <core/concepts/integral.hpp>
#include <core/traits/is_ptr.hpp>

namespace core {
template <typename T>
struct hash_impl;

template <typename T> requires integral<T> || is_ptr<T>
struct hash_impl<T> {
    constexpr u64 operator()(const T& value) const {
        return fnv1a64(&value, sizeof(value));
    }
};

template <typename T>
constexpr u64 hash(const T& value) {
    return hash_impl<T>{}(value);
}
} // namespace core
