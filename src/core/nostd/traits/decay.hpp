#pragma once

#include "declval.hpp"

namespace core {
template <typename T>
using decay = decltype(auto(declval<T>()));
} // namespace core


#if 0
#include "is_array.hpp"
#include "remove_extent.hpp"
#include "remove_ref.hpp"
#include "remove_cv.hpp"
#include "is_function.hpp"
#include "add_pointer.hpp"

namespace core {
namespace details_traits {
    template <typename T>
    struct decay {
        using type = typename remove_cv<T>::type;
    };
    template <typename T> requires is_array<T>::value
    struct decay<T> {
        using type = typename details_traits::remove_extent<T>::type*;
    };
    template <typename T> requires is_function<T>
    struct decay<T> {
        using type = typename add_pointer<T>::type;
    };
} // namespace details_traits
template <typename T>
using decay = typename details_traits::decay<remove_ref<T>>::type;

} // namespace core
#endif
