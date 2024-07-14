#pragma once

#include <cstddef>
#include <type_traits>

#include <core/meta/type.hpp>

namespace core::lh {
template <size_t k>
struct tag {
    friend auto get(tag<k>);
};

template <size_t k, auto r = 0>
struct def {
    friend auto get(tag<k>) {
        return r;
    }
};

template <size_t key, size_t N, typename = decltype([] {})>
constexpr auto find() {
    if constexpr (requires { get(tag<key>{}); })
        return find<key + N, N>();
    else
        return key;
}

#define fnd(MAX, ...) find<__VA_ARGS__, MAX, u>() - MAX
template <typename u = decltype([] {})>
constexpr auto count() {
    constexpr auto c = find<fnd(4, fnd(8, fnd(16, fnd(32, fnd(64, fnd(128, 252)))))), 1, u>();
    static_assert(!requires { def<c>::get; });
    return c;
}
#undef fnd

template <size_t k>
struct map_tag {
    friend auto map_get(map_tag<k>);
};

template <size_t k, typename T>
struct map_def {
    friend auto map_get(map_tag<k>) {
        return type<T>;
    }
};

template <size_t i, typename T>
constexpr void def_type_idx() {
    static_assert(!requires { map_def<i, T>::map_get; });
}

template <size_t i>
constexpr auto get_type_by_idx() {
    return map_get(map_tag<i>{});
}

template <size_t s = 0>
void type_idx_dispatch(size_t idx, auto&& f) {
#define def_case(n)                                            \
    case s + n:                                                \
        if constexpr (requires { map_get(map_tag<s + n>{}); }) \
            f(get_type_by_idx<s + n>());                       \
        else \
            return; \
        break

    switch (s + idx) {
        def_case(0);
        def_case(1);
        def_case(2);
        def_case(3);
        def_case(4);
        def_case(5);
        def_case(6);
        def_case(7);
        def_case(8);
        def_case(9);

#undef def_case

    default: return type_idx_dispatch<10>(idx - 10, static_cast<decltype(f)>(f));
    }
}
} // namespace core::lh
