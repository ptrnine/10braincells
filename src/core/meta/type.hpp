#pragma once

#include <core/traits/is_array.hpp>
#include <core/traits/is_same.hpp>
#include <core/int_const.hpp>

namespace core {

template <size_t I, typename T>
struct indexed_t {
    static inline constexpr auto index = I;
    using type = T;
};

template <typename T>
struct ctor_caller {};

template <typename T> requires(!is_array<T>)
struct ctor_caller<T> {
    static constexpr T make(auto&&... args) {
        return T{static_cast<decltype(args)>(args)...};
    }
};

template <typename T>
struct type_t : ctor_caller<T> {
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

template <size_t I, typename T>
struct type_t<indexed_t<I, T>> {
    static inline constexpr int_const<I> index() {
        return {};
    }

    static inline constexpr type_t<T> type() {
        return {};
    }

    template <typename U>
    constexpr auto operator==(type_t<U>) const {
        return is_same<indexed_t<I, T>, U>;
    }

    template <typename U>
    constexpr auto operator!=(type_t<U>) const {
        return !is_same<indexed_t<I, T>, U>;
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
