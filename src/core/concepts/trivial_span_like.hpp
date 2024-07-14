#pragma once

#include <core/concepts/ptr_to_trivial.hpp>
#include <core/concepts/convertible_to.hpp>

namespace core
{
template <typename T>
concept trivial_span_like = (!core::trivial<T>) && requires(const T& v) {
    { v.data() } -> ptr_to_trivial;
    { v.size() } -> convertible_to<decltype(sizeof(0))>;
};
} // namespace core
