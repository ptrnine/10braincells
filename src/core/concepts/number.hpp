#pragma once

#include "integral.hpp"
#include "floating_point.hpp"

namespace core
{
template <typename T>
concept number = integral<T> || floating_point<T>;
}
