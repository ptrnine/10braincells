#pragma once

namespace core {
template <typename T>
inline constexpr auto is_const = false;
template <typename T>
inline constexpr auto is_const<const T> = true;
} // namespace core
