#pragma once

namespace core {
template <typename T1, typename T2>
static inline constexpr bool is_same = __is_same(T1, T2);
template <typename T, typename... Ts>
static inline constexpr bool is_any_same = (is_same<T, Ts> || ... || false);
}
