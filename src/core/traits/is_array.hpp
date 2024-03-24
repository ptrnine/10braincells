#pragma once

namespace core {
namespace details_traits {
    template <typename>
    struct is_array {
        static inline constexpr bool value = false;
    };
    template <typename T, decltype(sizeof(0)) S>
    struct is_array<T[S]> {
        static inline constexpr bool value = true;
    };
    template <typename T>
    struct is_array<T[]> {
        static inline constexpr bool value = true;
    };
} // namespace details_traits
template <typename T>
inline constexpr auto is_array = details_traits::is_array<T>::value;

template <typename>
inline constexpr bool is_bounded_array = true;

template <typename T>
inline constexpr bool is_bounded_array<T[]> = false;

} // namespace core
