#pragma once

namespace core
{
template <typename T>
static inline constexpr bool is_enum = __is_enum(T);
}
