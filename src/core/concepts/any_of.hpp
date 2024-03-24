#pragma once

#include "same_as.hpp"

namespace core {
template <typename T, typename... Ts>
concept any_of = (same_as<T, Ts> || ... || false);
}
