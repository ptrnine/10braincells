#pragma once

namespace core {
namespace details_traits {
    template <typename T>
    struct remove_cv {
        using type = T;
    };

    template <typename T>
    struct remove_cv<const T> {
        using type = T;
    };
    template <typename T>
    struct remove_cv<volatile T> {
        using type = T;
    };
    template <typename T>
    struct remove_cv<const volatile T> {
        using type = T;
    };
} // namespace details_traits
template <typename T>
using remove_cv = typename details_traits::remove_cv<T>::type;
} // namespace core
