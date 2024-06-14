#pragma once

#include <core/traits/is_same.hpp>

namespace core {
template <typename T>
struct type_t {
    using type = T;

    static constexpr type_t operator()() {
        return {};
    }

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

template <auto t>
using decl_type = typename decltype(t)::type;
} // namespace core
