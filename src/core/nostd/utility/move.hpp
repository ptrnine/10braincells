#pragma once

#include "../traits/remove_ref.hpp"

namespace core {
template <typename T>
constexpr decltype(auto) move(T&& v) {
    return static_cast<remove_ref<T>&&>(v);
}
} // namespace core
