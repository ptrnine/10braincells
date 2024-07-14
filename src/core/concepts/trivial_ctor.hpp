#pragma once

#include "../traits/add_ref.hpp"
#include "ctor.hpp"

namespace core {
template <typename T, typename... Arg>
concept trivial_ctor = ctor<T, Arg...> && bool(__is_trivially_constructible(T, Arg...));

template <typename T>
concept trivial_move_ctor = move_ctor<T> && trivial_ctor<T, add_rvalue_ref<T>>;

template <typename T>
concept trivial_copy_ctor = copy_ctor<T> && trivial_ctor<T, add_lvalue_ref<const T>>;
} // namespace core
