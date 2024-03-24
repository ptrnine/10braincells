#pragma once

#include "convertible_to.hpp"

namespace core {
template <typename T>
concept empty_class = __is_empty(T);

template <typename T>
concept final_class = __is_final(T);

template <typename Base, typename Derived>
concept base_of = bool(__is_base_of(Base, Derived));

template <typename Derived, typename Base>
concept derived_from = bool(__is_base_of(Base, Derived) && convertible_to<const volatile Derived*, const volatile Base*>);
} // namespace core
