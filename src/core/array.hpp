#pragma once

#include <core/basic_types.hpp>

#define fwd(x) static_cast<decltype(x)>(x)

namespace core {
template <typename T, size_t... Sz>
struct array;

namespace details {
    template <typename T, size_t... Sz>
    struct array_type {
        using type = T;
    };
    template <typename T, size_t S, size_t... Sz>
    struct array_type<T, S, Sz...> : array_type<T[S], Sz...> {};

    template <typename T, size_t S, size_t... Sz>
    struct array_type<T&, S, Sz...> : array_type<T(&)[S], Sz...> {};

    template <typename>
    struct is_array_ref {
        static inline constexpr bool value = false;
    };

    template <typename T, size_t S>
    struct is_array_ref<T(&)[S]> {
        static inline constexpr bool value = true;
    };

    template <typename T, size_t S>
    struct is_array_ref<T(&&)[S]> {
        static inline constexpr bool value = true;
    };

    template <typename>
    struct current_extent;

    template <typename T, size_t S>
    struct current_extent<T[S]> {
        static inline constexpr size_t value = S;
    };

    template <typename T, size_t S>
    struct current_extent<T(&)[S]> {
        static inline constexpr size_t value = S;
    };

    template <typename T, size_t S>
    struct current_extent<T(&&)[S]> {
        static inline constexpr size_t value = S;
    };

    static inline constexpr decltype(auto) subscript(auto&& arr, auto&& sz, auto&&... szs) {
        if constexpr (sizeof...(szs))
            return subscript(fwd(arr)[fwd(sz)], fwd(szs)...);
        else if constexpr (is_array_ref<decltype(fwd(arr)[fwd(sz)])>::value)
            return array{fwd(arr)[fwd(sz)]};
        else
            return fwd(arr)[fwd(sz)];
    }
} // namespace details

template <typename T, size_t... Sz>
struct array {
    using type       = T;
    using array_type = typename details::array_type<T, Sz...>::type;

    static inline constexpr auto has_storage = !details::is_array_ref<array_type>::value;

    static inline constexpr auto dimm_size() noexcept {
        return 0 + (Sz * ...);
    }

    static inline constexpr auto size() noexcept {
        return details::current_extent<array_type>::value;
    }

    constexpr decltype(auto) operator[](auto&&... idx) const& noexcept {
        return details::subscript(c_array, fwd(idx)...);
    }

    constexpr decltype(auto) operator[](auto&&... idx) & noexcept {
        return details::subscript(c_array, fwd(idx)...);
    }

    constexpr decltype(auto) operator[](auto&&... idx) && noexcept {
        return details::subscript(static_cast<array_type&&>(c_array), fwd(idx)...);
    }

    constexpr bool operator==(const array& arr) const {
        for (size_t i = 0; i < size(); ++i)
            if (c_array[i] != arr.c_array[i])
                return false;
        return true;
    }

    constexpr bool operator!=(const array& arr) const {
        return !(*this == arr);
    }
#undef fwd

#define def_member(NAME, ...) \
    constexpr auto NAME() noexcept { __VA_ARGS__ } \
    constexpr auto NAME() const noexcept { __VA_ARGS__ }

#define def_ref_member(NAME, ...) \
    constexpr decltype(auto) NAME() & noexcept { return __VA_ARGS__; } \
    constexpr decltype(auto) NAME() const& noexcept { return __VA_ARGS__; }
    //constexpr decltype(auto) NAME() && noexcept { return static_cast<decltype(__VA_ARGS__)&&>(__VA_ARGS__); }

    def_member(data, return &details::subscript(c_array, ((void)Sz, 0)...);)
    def_member(begin, return &c_array[0];)
    def_member(end, return &c_array[0] + size();)
    def_ref_member(front, operator[](0))
    def_ref_member(back, operator[](size() - 1))
#undef def_member

    array_type c_array;
};

template <typename T, typename... Ts>
array(T, Ts...) -> array<T, sizeof...(Ts) + 1>;

template <typename T, size_t S>
array(T(&)[S]) -> array<T&, S>;

template <typename T, size_t S1, size_t S2>
array(T(&)[S2][S1]) -> array<T&, S1, S2>;

template <typename T, size_t S1, size_t S2, size_t S3>
array(T(&)[S3][S2][S1]) -> array<T&, S1, S2, S3>;

template <typename T, size_t S1, size_t S2, size_t S3, size_t S4>
array(T(&)[S4][S3][S2][S1]) -> array<T&, S1, S2, S3, S4>;

template <size_t i, typename T, size_t... Sz>
constexpr decltype(auto) get(core::array<T, Sz...>& array) {
    return array.c_array[i];
}

template <size_t i, typename T, size_t... Sz>
constexpr decltype(auto) get(const core::array<T, Sz...>& array) {
    return array.c_array[i];
}

template <size_t i, typename T, size_t... Sz>
constexpr decltype(auto) get(core::array<T, Sz...>&& array) {
    return static_cast<decltype(array.c_array[i])&&>(array.c_array[i]);
}
} // namespace core

namespace std {
template <typename>
struct tuple_size;

template <size_t, typename>
struct tuple_element;

template <typename T, size_t... Sz>
struct tuple_size<core::array<T, Sz...>> {
    static inline constexpr size_t value = core::array<T, Sz...>::size();
};

template <size_t i, typename T, size_t... Sz>
struct tuple_element<i, core::array<T, Sz...>> {
    using type = decltype(core::array<T, Sz...>().c_array[0]);
};
} // namespace std

#undef fwd
