#pragma once

#include <core/traits/is_enum.hpp>

namespace core
{
template <typename T> requires is_enum<T>
using underlying_type = __underlying_type(T);
}
