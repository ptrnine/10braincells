#pragma once

namespace core {
template <typename T>
inline constexpr bool is_class = __is_class(T);
} // namespace core
