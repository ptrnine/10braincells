#pragma once

#include <core/concepts/convertible_to.hpp>

namespace core
{
template <typename T>
concept sized_const_str = requires(const T& str) {
    { str.data() } -> convertible_to<const char*>;
    { str.size() } -> convertible_to<decltype(sizeof(0))>;
};
} // namespace core
