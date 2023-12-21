#pragma once

#include "false_on_eval.hpp"

namespace core {
namespace details_traits {
    /* Add rref */
    template <typename T, typename I = T&&>
    I declval(int);
    template <typename T>
    T declval(long);
} // namespace details_traits

template <typename T>
auto declval() noexcept -> decltype(details_traits::declval<T>(0)) {
    static_assert(details::false_on_eval<T>::false_, "declval() must be used in unevaluated constext only");
    return details_traits::declval<T>(0);
}
} // namespace core
