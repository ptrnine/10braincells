#pragma once

#include "../traits/remove_ref.hpp"

namespace core {
template <typename T>
constexpr decltype(auto) move(T&& v) {
    return static_cast<remove_ref<T>&&>(v);
}
/* To prevent ambiguity with std::move */
template <typename T>
constexpr decltype(auto) mov(T&& v) {
    return static_cast<remove_ref<T>&&>(v);
}
} // namespace core
