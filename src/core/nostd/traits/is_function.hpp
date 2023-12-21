#pragma once

#include "is_const.hpp"

namespace core {
template <typename T>
inline constexpr auto is_function = !is_const<const T>;
template <typename T>
inline constexpr auto is_function<T&> = false;
template <typename T>
inline constexpr auto is_function<T&&> = false;
} // namespace core
