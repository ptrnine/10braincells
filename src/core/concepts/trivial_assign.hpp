#pragma once

#include "../traits/add_ref.hpp"
#include "assign.hpp"

namespace core {
template <typename T, typename Arg>
concept trivial_assign = assign<T, Arg> && bool(__is_trivially_assignable(T, Arg));

template <typename T>
concept trivial_move_assign = move_assign<T> && trivial_assign<add_lvalue_ref<T>, add_rvalue_ref<T>>;

template <typename T>
concept trivial_copy_assign = copy_assign<T> && trivial_assign<add_lvalue_ref<T>, add_lvalue_ref<const T>>;
} // namespace core
