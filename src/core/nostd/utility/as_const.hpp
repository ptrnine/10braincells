#pragma once

#include "../traits/add_const.hpp"

namespace core
{
template <typename T>
constexpr add_const<T>& as_const(T& v) noexcept {
    return v;
}
template <typename T>
void as_const(const T&&) = delete;
} // namespace core
