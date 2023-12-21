#pragma once

#include "../basic_types.hpp"
#include "convertible_to.hpp"

namespace core {
template <typename T>
concept variant = requires (T v) {
    {v.valueless_by_exception()} -> convertible_to<bool>;
    {v.index()} -> convertible_to<size_t>;
};
}
