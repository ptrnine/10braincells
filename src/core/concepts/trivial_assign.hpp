#pragma once

#include "../traits/add_ref.hpp"

namespace core {
template <typename T, typename Arg>
concept trivial_assign = bool(__is_trivially_assignable(T, Arg));

template <typename T>
concept trivial_move_assign = trivial_assign<add_lvalue_ref<T>, add_rvalue_ref<T>>;

template <typename T>
concept trivial_copy_assign = trivial_assign<add_lvalue_ref<T>, add_lvalue_ref<const T>>;
} // namespace core
