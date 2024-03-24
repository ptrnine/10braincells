#pragma once

#include "../traits/add_ref.hpp"

namespace core {
template <typename T, typename... Arg>
concept trivial_ctor = bool(__is_trivially_constructible(T, Arg...));

template <typename T>
concept trivial_move_ctor = trivial_ctor<T, add_rvalue_ref<T>>;

template <typename T>
concept trivial_copy_ctor = trivial_ctor<T, add_lvalue_ref<const T>>;
} // namespace core
