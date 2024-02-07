#pragma once

#include "../traits/remove_ref.hpp"

namespace core {
template <typename T>
constexpr T&& forward(remove_ref<T>& v) noexcept {
    return static_cast<T&&>(v);
}
template <typename T>
constexpr T&& forward(remove_ref<T>&& v) noexcept {
    return static_cast<T&&>(v);
}

/* To prevent ambiguity with std::forward */
template <typename T>
constexpr T&& fwd(remove_ref<T>& v) noexcept {
    return static_cast<T&&>(v);
}
template <typename T>
constexpr T&& fwd(remove_ref<T>&& v) noexcept {
    return static_cast<T&&>(v);
}
} // namespace core
