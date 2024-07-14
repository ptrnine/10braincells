#pragma once

#include <cmath>

/* TODO: switch to core::floating_point */
#include <concepts>

namespace core
{
inline bool approx_equal(std::floating_point auto a, std::floating_point auto b, std::floating_point auto epsilon) {
    return std::fabs(a - b) <= ((std::fabs(a) < std::fabs(b) ? std::fabs(b) : std::fabs(a)) * epsilon);
}

inline bool
essentially_equal(std::floating_point auto a, std::floating_point auto b, std::floating_point auto epsilon) {
    return std::fabs(a - b) <= ((std::fabs(a) > std::fabs(b) ? std::fabs(b) : std::fabs(a)) * epsilon);
}

inline bool
definitely_greater(std::floating_point auto a, std::floating_point auto b, std::floating_point auto epsilon) {
    return (a - b) > ((std::fabs(a) < std::fabs(b) ? std::fabs(b) : std::fabs(a)) * epsilon);
}

inline bool definitely_less(std::floating_point auto a, std::floating_point auto b, std::floating_point auto epsilon) {
    return (b - a) > ((std::fabs(a) < std::fabs(b) ? std::fabs(b) : std::fabs(a)) * epsilon);
}
} // namespace core
