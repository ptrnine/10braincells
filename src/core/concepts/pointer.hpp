#pragma once

#include <core/traits/is_ptr.hpp>

namespace core
{
template <typename T>
concept pointer = is_ptr<T>;
}
