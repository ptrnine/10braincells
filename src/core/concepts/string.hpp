#pragma once

#include <string>

#include <core/concepts/convertible_to.hpp>
#include <core/concepts/char_type.hpp>

namespace core
{
namespace details {
    template <typename T>
    static inline constexpr bool _c_string = false;

    template <char_type C>
    static inline constexpr bool _c_string<C*> = true;

    template <char_type C>
    static inline constexpr bool _c_string<const C*> = true;

    template <char_type C, size_t S>
    static inline constexpr bool _c_string<const C(&)[S]> = true;

    template <typename T>
    static inline constexpr bool _std_string = false;

    template <typename T, typename Traits, typename Alloc>
    static inline constexpr bool _std_string<std::basic_string<T, Traits, Alloc>> = true;
} // namespace details

template <typename T>
concept c_string = details::_c_string<T>;

template <typename T>
concept sized_str = requires(const T& str) {
    { str.data() } -> c_string;
    { str.size() } -> convertible_to<decltype(sizeof(0))>;
};

template <typename T, typename C>
concept sized_str_typed = char_type<C> && requires(const T& str) {
    { str.data() } -> c_string;
    { *str.data() } -> convertible_to<C>;
    { str.size() } -> convertible_to<decltype(sizeof(0))>;
};

template <typename T>
concept null_term_string = c_string<T> || details::_std_string<T>;
} // namespace core
