#pragma once

#include "../traits/add_ref.hpp"

namespace core {
template <typename T, typename... Arg>
concept nothrow_ctor = bool(__is_nothrow_constructible(T, Arg...));

template <typename T>
concept nothrow_move_ctor = nothrow_ctor<T, add_rvalue_ref<T>>;

template <typename T>
concept nothrow_copy_ctor = nothrow_ctor<T, add_lvalue_ref<const T>>;
} // namespace core
