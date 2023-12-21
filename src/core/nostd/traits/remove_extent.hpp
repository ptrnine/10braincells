#pragma once

#include "../basic_types.hpp"

namespace core {
namespace details_traits {
    template <typename T>
    struct remove_sized_extent {
        using type = T;
    };
    template <typename T, size_t S>
    struct remove_sized_extent<T[S]> {
        using type = T;
    };

    template <typename T>
    struct remove_extent {
        using type = typename remove_sized_extent<T>::type;
    };
    template <typename T>
    struct remove_extent<T[]> {
        using type = T;
    };

    template <typename T>
    struct remove_all_sized_extents {
        using type = T;
    };
    template <typename T, size_t S>
    struct remove_all_sized_extents<T[S]> {
        using type = typename remove_all_sized_extents<T>::type;
    };

    template <typename T>
    struct remove_all_extents {
        using type = typename remove_all_sized_extents<T>::type;
    };
    template <typename T>
    struct remove_all_extents<T[]> {
        using type = typename remove_all_extents<T>::type;
    };
} // namespace details_traits

template <typename T>
using remove_sized_extent = typename details_traits::remove_sized_extent<T>::type;
template <typename T>
using remove_extent = typename details_traits::remove_extent<T>::type;
template <typename T>
using remove_all_extents = typename details_traits::remove_all_extents<T>::type;
template <typename T>
using remove_all_sized_extents = typename details_traits::remove_all_sized_extents<T>::type;

} // namespace core
