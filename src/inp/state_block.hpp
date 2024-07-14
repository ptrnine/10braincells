#pragma once

#include <atomic>

namespace inp
{
template <typename T>
struct state_block {
    using type                               = T;
    static inline constexpr size_t type_size = sizeof(T);
    static inline constexpr bool   is_atomic = false;

    constexpr T load() const {
        return _block;
    }

    constexpr void fetch_add(T val) {
        _block += val;
    }

    constexpr void fetch_or(T val) {
        _block |= val;
    }

    constexpr void fetch_and(T val) {
        _block &= val;
    }

    constexpr T exchange(T new_val) {
        auto old = _block;
        _block   = new_val;
        return old;
    }

    T _block = 0;
};

template <typename T>
struct state_block<std::atomic<T>> {
    using type                               = T;
    static inline constexpr size_t type_size = sizeof(T);
    static inline constexpr bool   is_atomic = true;

    constexpr T load() const {
        return _block.load(std::memory_order_relaxed);
    }

    constexpr void fetch_add(T val) {
        _block.fetch_add(val, std::memory_order_relaxed);
    }

    constexpr void fetch_or(T val) {
        _block.fetch_or(val, std::memory_order_relaxed);
    }

    constexpr void fetch_and(T val) {
        _block.fetch_and(val, std::memory_order_relaxed);
    }

    constexpr T exchange(T new_val) {
        return _block.exchange(new_val);
    }

    alignas(64) std::atomic<T> _block = 0;
};
} // namespace inp
