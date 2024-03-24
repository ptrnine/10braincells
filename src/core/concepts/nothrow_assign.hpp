#pragma once

#include "../traits/add_ref.hpp"

namespace core {
template <typename T, typename Arg>
concept nothrow_assign = bool(__is_nothrow_assignable(T, Arg));

template <typename T>
concept nothrow_move_assign = nothrow_assign<add_lvalue_ref<T>, add_rvalue_ref<T>>;

template <typename T>
concept nothrow_copy_assign = nothrow_assign<add_lvalue_ref<T>, add_lvalue_ref<const T>>;
} // namespace core
