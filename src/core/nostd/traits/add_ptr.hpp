#pragma once

#include "../concepts/referenceable.hpp"
#include "remove_ref.hpp"

namespace core {
namespace details_traits {
    template <typename T>
    struct add_pointer {
        using type = T;
    };
    template <typename T> requires referenceable<T> || __is_same(T, void)
    struct add_pointer<T> {
        using type = remove_ref<T>::type*;
    };
} // namespace details_traits

template <typename T>
using add_ptr = typename details_traits::add_pointer<T>::type;
} // namespace core
