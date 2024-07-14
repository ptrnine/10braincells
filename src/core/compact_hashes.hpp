#pragma once

#include "basic_types.hpp"

namespace core
{
inline constexpr u64 fnv1a64(const void* ptr, size_t size) {
    auto p    = static_cast<const u8*>(ptr);
    u64  hash = 0xcbf29ce484222325;

    while (size--) hash = (hash ^ *p++) * 0x100000001b3;

    return hash;
}

inline constexpr u64 fnv1a64_nullterm(const void* ptr) {
    auto p    = static_cast<const u8*>(ptr);
    u64  hash = 0xcbf29ce484222325;

    while (*p) hash = (hash ^ *p++) * 0x100000001b3;

    return hash;
}
} // namespace dfdh
