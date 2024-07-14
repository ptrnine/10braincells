#pragma once

namespace core {
template <typename T>
inline constexpr bool is_ref = false;
template <typename T>
inline constexpr bool is_ref<T&> = true;
template <typename T>
inline constexpr bool is_ref<T&&> = true;
template <typename T>
inline constexpr bool is_rref = false;
template <typename T>
inline constexpr bool is_rref<T&&> = true;
} // namespace core
