#pragma once

#include "../traits/is_same.hpp"

namespace core {
template <typename T>
struct type_t {
    using type = T;

    constexpr type operator+() const noexcept;

    template <typename U>
    constexpr auto operator==(type_t<U>) const {
        return is_same<T, U>;
    }

    template <typename U>
    constexpr auto operator!=(type_t<U>) const {
        return !is_same<T, U>;
    }
};

template <typename T>
static inline constexpr type_t<T> type;

template <typename T>
constexpr auto type_of(const T&) {
    return type<T>;
}
} // namespace core
