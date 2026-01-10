#pragma once

#include <core/traits/remove_cvref.hpp>

namespace core::io {
template <typename T>
concept in_constraint = requires { typename remove_cvref<T>::in_tag; };

template <typename T>
concept out_constraint = requires { typename remove_cvref<T>::out_tag; };
} // namespace core::io
