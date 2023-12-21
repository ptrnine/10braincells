#pragma once

namespace core {
namespace details_traits {
    template <typename T>
    struct remove_const {
        using type = T;
    };
    template <typename T>
    struct remove_const<const T> {
        using type = T;
    };
} // namespace details_traits

template <typename T>
using remove_const = typename details_traits::remove_const<T>::type;
} // namespace core
