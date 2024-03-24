#pragma once

#include "../traits/add_ref.hpp"

namespace core {
template <typename T, typename Arg>
concept assign = bool(__is_assignable(T, Arg));

template <typename T>
concept move_assign = assign<add_lvalue_ref<T>, add_rvalue_ref<T>>;

template <typename T>
concept copy_assign = assign<add_lvalue_ref<T>, add_lvalue_ref<const T>>;
} // namespace core
