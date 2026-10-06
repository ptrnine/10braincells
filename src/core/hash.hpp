#pragma once

#include <cmath>
#include <string>
#include <string_view>
#include <vector>

#include <core/array.hpp>
#include <core/basic_types.hpp>
#include <core/byteview.hpp>
#include <core/compact_hashes.hpp>
#include <core/concepts/floating_point.hpp>
#include <core/concepts/integral.hpp>
#include <core/concepts/string.hpp>
#include <core/concepts/trivial.hpp>
#include <core/opt.hpp>
#include <core/traits/is_ptr.hpp>
#include <core/var.hpp>
#include <core/xxhash.hpp>

namespace core {
template <typename T>
struct hash_impl;

/*
 * Raw values: integrals and pointers are hashed by their bytes / address.
 * c-strings are handled separately (by content), floats are canonicalized,
 * and trivially-copyable types fall back to a byte hash at the end.
 */
template <typename T> requires integral<T> || (is_ptr<T> && !c_string<T>)
struct hash_impl<T> {
    constexpr u64 operator()(const T& value) const {
        return fnv1a64(&value, sizeof(value));
    }
};

/* C-strings: hash by content, not by address (the primary template hashes pointers by address). */
template <typename T> requires c_string<T>
struct hash_impl<T> {
    constexpr u64 operator()(const T& str) const {
        return fnv1a64_nullterm(str);
    }
};

/*
 * Floating point: hash by bytes after canonicalization, so that equal values
 * have equal hashes: every NaN bit pattern folds to one, and -0.0 folds to +0.0.
 */
template <typename T> requires floating_point<T>
struct hash_impl<T> {
    constexpr u64 operator()(const T& value) const {
        if (std::isnan(value)) {
            u8 canonical[sizeof(T)] = {};
            for (size_t i = 0; i < sizeof(T); ++i)
                canonical[i] = 0xFF;
            return fnv1a64(canonical, sizeof(T));
        }
        if (value == T{}) { // -0.0 == +0.0
            u8 zeros[sizeof(T)] = {};
            return fnv1a64(zeros, sizeof(T));
        }
        return fnv1a64(&value, sizeof(value));
    }
};

/* byteview: one-shot xxHash over the view's bytes. */
template <size_t S>
struct hash_impl<byteview<S>> {
    u64 operator()(const byteview<S>& v) const {
        return xxh64(v.data(), v.size());
    }
};

/*
 * Fallback for trivially-copyable types (POD structs, util::vec*, ...):
 * plain byte hash. Excludes the types handled above; more specialized
 * partials (vector, opt, var, array, ...) still win by partial ordering.
 */
template <typename T>
requires (trivial<T> && !integral<T> && !is_ptr<T> && !floating_point<T> && !c_string<T>)
struct hash_impl<T> {
    constexpr u64 operator()(const T& value) const {
        return fnv1a64(&value, sizeof(value));
    }
};

template <typename T>
constexpr u64 hash(const T& value) {
    return hash_impl<T>{}(value);
}

template <typename CharT, typename Traits, typename Alloc>
struct hash_impl<std::basic_string<CharT, Traits, Alloc>> {
    u64 operator()(const std::basic_string<CharT, Traits, Alloc>& str) const {
        return xxh64(str.data(), (u64)str.size() * sizeof(CharT));
    }
};

template <typename CharT, typename Traits>
struct hash_impl<std::basic_string_view<CharT, Traits>> {
    u64 operator()(std::basic_string_view<CharT, Traits> str) const {
        return xxh64(str.data(), (u64)str.size() * sizeof(CharT));
    }
};

template <typename T, typename Alloc>
struct hash_impl<std::vector<T, Alloc>> {
    u64 operator()(const std::vector<T, Alloc>& vec) const {
        xxhash64 state;
        for (const auto& elem : vec) {
            u64 h  = core::hash(elem);
            state.update(&h, sizeof(h));
        }
        u64 len = (u64)vec.size();
        state.update(&len, sizeof(len));
        return state.digest();
    }
};

template <typename T>
struct hash_impl<opt<T>> {
    u64 operator()(const opt<T>& value) const {
        xxhash64 state;
        u64 flag = value.has_value();
        state.update(&flag, sizeof(flag));
        if (value.has_value()) {
            u64 h = core::hash(*value);
            state.update(&h, sizeof(h));
        }
        return state.digest();
    }
};

template <typename... Ts>
struct hash_impl<var<Ts...>> {
    u64 operator()(const var<Ts...>& value) const {
        xxhash64 state;
        u64 idx = value.index();
        state.update(&idx, sizeof(idx));
        visit(value, [&](const auto& alt) {
            u64 h = core::hash(alt);
            state.update(&h, sizeof(h));
        });
        return state.digest();
    }
};

template <typename T, size_t... Sz>
struct hash_impl<array<T, Sz...>> {
    u64 operator()(const array<T, Sz...>& value) const {
        xxhash64 state;
        const auto* data = value.data();
        size_t     total  = 0 + (Sz * ...);
        for (size_t i = 0; i < total; ++i) {
            u64 h  = core::hash(data[i]);
            state.update(&h, sizeof(h));
        }
        u64 len = (u64)total;
        state.update(&len, sizeof(len));
        return state.digest();
    }
};

} // namespace core
