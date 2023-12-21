#pragma once

#if !__has_builtin(__is_trivially_destructible)
#include "../traits/is_destructible.hpp"
#endif

namespace core {
#if __has_builtin(__is_trivially_destructible)
template <typename T>
concept trivial_dtor = __is_trivially_destructible(T);
#else
template <typename T>
concept trivial_dtor = bool(__has_trivial_destructor(T)) && is_destructible<T>;
#endif
} // namespace core
