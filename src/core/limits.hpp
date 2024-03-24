#pragma once

#include "concepts/integral.hpp"

namespace core {
namespace details {
    template <typename T>
    static inline constexpr auto digits = sizeof(T) * __CHAR_BIT__ - signed_integral<T>;

    template <auto Min, auto Max>
    struct minmax_helper {
        static inline constexpr auto min() {
            return Min;
        }
        static inline constexpr auto max() {
            return Max;
        }
    };

    template <typename T>
    struct minmax_default {
        static inline constexpr T min() {
            return signed_integral<T> ? T(1) << digits<T> : T(0);
        }
        static inline constexpr T max() {
            return signed_integral<T> ? T((((T(1) << (digits<T> - 1)) - 1) << 1) + 1) : ~T(0);
        }
    };
} // namespace details

template <typename>
struct limits;

template <>
struct limits<bool> : details::minmax_helper<false, true> {};

template <integral T>
struct limits<T> : details::minmax_default<T> {};
} // namespace core
