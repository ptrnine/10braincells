#pragma once

#include "is_function.hpp"
#include "remove_extent.hpp"

namespace core {
namespace dtls {
    template <typename T, bool = is_function<T>>
    struct is_destructible;

    template <typename T>
    struct is_destructible<T, false> {
        static inline constexpr bool value = requires(T & v) { v.~T(); };
    };

    template <typename T>
    struct is_destructible<T, true> {
        static inline constexpr bool value = false;
    };
} // namespace dtls

template <typename T>
inline constexpr bool is_destructible = dtls::is_destructible<remove_all_extents<T>>::value;

template <typename T>
inline constexpr bool is_destructible<T[]> = false;

template <>
inline constexpr bool is_destructible<void> = false;

template <typename T>
inline constexpr bool is_destructible<T&> = true;

template <typename T>
inline constexpr bool is_destructible<T&&> = true;
} // namespace core
