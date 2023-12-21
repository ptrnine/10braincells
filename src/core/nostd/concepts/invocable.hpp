#pragma once

#include "convertible_to.hpp"

namespace core {
template <typename T, typename... Args>
concept invocable = requires (T f, Args&&... args) { f(static_cast<decltype(args)>(args)...); };

template <typename R, typename T, typename... Args>
concept invocable_r = requires (T f, Args&&... args) {
    {f(static_cast<decltype(args)>(args)...)} -> convertible_to<R>;
};
} // namespace core
