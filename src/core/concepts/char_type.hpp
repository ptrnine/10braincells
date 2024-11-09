#pragma once

#include <core/concepts/same_as.hpp>
#include <core/concepts/any_of.hpp>

namespace core
{
template <typename T>
concept char_type = core::any_of<T, char, wchar_t, char8_t, char16_t, char32_t>;
}
