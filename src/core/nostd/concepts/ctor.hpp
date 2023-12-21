#pragma once

#include "../traits/add_ref.hpp"

namespace core {
template <typename T, typename... Args>
concept ctor = bool(__is_constructible(T, Args...));

template <typename T>
concept default_ctor = ctor<T>;

template <typename T>
concept copy_ctor = ctor<T, add_lvalue_ref<const T>>;

template <typename T>
concept move_ctor = ctor<T, add_rvalue_ref<T&&>>;
}
