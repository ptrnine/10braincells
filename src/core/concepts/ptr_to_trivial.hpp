#pragma once

#include <core/concepts/trivial.hpp>
#include <core/traits/is_ptr.hpp>

namespace core {
template <typename T>
concept ptr_to_trivial = core::is_ptr<T> && requires(T v) {
    { auto(*v) } -> core::trivial;
};
} // namespace core
