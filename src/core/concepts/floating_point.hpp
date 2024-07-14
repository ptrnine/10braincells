#pragma once

#include "any_of.hpp"
#include "../traits/remove_cv.hpp"

namespace core
{
template <typename T>
concept floating_point = any_of<remove_cv<T>, float, double, long double>;
}
