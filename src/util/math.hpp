#pragma once

#include <cmath>

#include <glm/gtc/constants.hpp>

#include <core/concepts/floating_point.hpp>
#include <util/vec.hpp>

namespace util
{
/* Constraint angle [-PI ; +PI] */
template <core::floating_point T>
T constraint_pi(T angle) {
    angle = std::fmod(angle + glm::pi<T>(), glm::two_pi<T>());
    return angle < 0 ? angle + glm::pi<T>() : angle - glm::pi<T>();
}

/* Constraint angle [-PI ; +PI] */
template <core::floating_point T, size_t S>
vec<T, S> constraint_pi(const vec<T, S>& angles) {
    return vec_map(angles, [](T v) { return constraint_pi(v); });
}

template <core::floating_point T, size_t S>
vec<T, S> exp(const vec<T, S>& vec) {
    return vec_map(vec, [](T v) { return std::exp(v); });
}

template <core::floating_point T, size_t S>
vec<T, S> pow(const vec<T, S>& vec, T power) {
    return vec_map(vec, [power](T v) { return std::pow(v, power); });
}
} // namespace util
