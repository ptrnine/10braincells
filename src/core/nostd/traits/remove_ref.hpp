#pragma once

#include "remove_const.hpp"

namespace core {
namespace details_traits {
    template <typename T>
    struct remove_ref {
        using type = T;
    };
    template <typename T>
    struct remove_ref<T&> {
        using type = T;
    };
    template <typename T>
    struct remove_ref<T&&> {
        using type = T;
    };
    template <typename T>
    struct remove_rref {
        using type = T;
    };
    template <typename T>
    struct remove_rref<T&&> {
        using type = T;
    };
} // namespace details_traits

template <typename T>
using remove_ref = typename details_traits::remove_ref<T>::type;
template <typename T>
using remove_rref = typename details_traits::remove_rref<T>::type;
template <typename T>
using remove_const_ref = typename details_traits::remove_const<typename details_traits::remove_ref<T>::type>::type;
} // namespace core
