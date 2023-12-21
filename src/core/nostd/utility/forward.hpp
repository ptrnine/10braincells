#pragma once

#include "../traits/remove_ref.hpp"

#ifdef ENABLE_FORWARD_MACRO
#define fwd(what) static_cast<decltype(what)>(what)
#endif

namespace core {
template <typename T>
constexpr T&& forward(remove_ref<T>& v) noexcept {
    return static_cast<T&&>(v);
}
template <typename T>
constexpr T&& forward(remove_ref<T>&& v) noexcept {
    return static_cast<T&&>(v);
}
} // namespace core
