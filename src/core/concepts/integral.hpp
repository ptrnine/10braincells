#pragma once

#include "any_of.hpp"
#include "../traits/remove_cv.hpp"

namespace core {
template <typename T>
concept integral = any_of<remove_cv<T>,
                          bool,
                          char,
                          signed char,
                          unsigned char,
                          wchar_t,
                          char8_t,
                          char16_t,
                          char32_t,
                          short,
                          unsigned short,
                          int,
                          unsigned int,
                          long,
                          unsigned long,
                          long long,
                          unsigned long long,
                          __int128_t,
                          __uint128_t>;

template <typename T>
concept signed_integral = integral<T> && (T(-1) < 0);
}
