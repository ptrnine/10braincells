#pragma once

namespace core {
template <typename T>
inline constexpr bool is_ref = false;
template <typename T>
inline constexpr bool is_ref<T&> = true;
template <typename T>
inline constexpr bool is_ref<T&&> = true;
} // namespace core
