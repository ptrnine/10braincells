#pragma once

namespace core {
template <typename T>
concept empty_class = __is_empty(T);

template <typename T>
concept final_class = __is_final(T);

template <typename Base, typename Derived>
concept base_of = bool(__is_base_of(Base, Derived));
} // namespace core
