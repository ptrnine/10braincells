#pragma once


#if 0
namespace core {
#include "declval.hpp"

template <typename T>
using decay = decltype(auto(declval<T>()));
} // namespace core
#endif

#include "is_array.hpp"
#include "remove_extent.hpp"
#include "remove_ref.hpp"
#include "remove_cv.hpp"
#include "is_function.hpp"
#include "add_ptr.hpp"

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
        using type = add_ptr<T>;
    };
} // namespace details_traits

template <typename T>
using decay = typename details_traits::decay<remove_ref<T>>::type;

} // namespace core
