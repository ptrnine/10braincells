#pragma once

#include "int_parse.hpp"
#include "limits.hpp"

#include <core/traits/is_enum.hpp>
#include <core/traits/underlying_type.hpp>

namespace core {
#define BINOP(OP)                                                                                                      \
    template <auto c2>                                                                                                 \
    constexpr auto operator OP(int_const<c2> rhs) const {                                                              \
        return int_c<(c OP rhs)>;                                                                                      \
    }                                                                                                                  \
    constexpr auto operator OP(auto rhs) const { return c OP rhs; }

#define UNOP(OP)                                                                                                       \
    constexpr auto operator OP() const { return int_c<OP c>; }

template <auto>
struct int_const;

template <auto v>
static inline constexpr auto int_c = int_const<v>{};

template <typename T, auto v>
static inline constexpr auto int_t_c = int_const<static_cast<T>(v)>{};

template <auto c>
struct int_const {
    using type = decltype(c);
    static constexpr auto value = c;

    constexpr operator auto() const {
        return c;
    }

    constexpr int_const to_int() const {
        return {};
    }

    BINOP(+)
    BINOP(-)
    BINOP(*)
    BINOP(/)
    BINOP(%)
    BINOP(&&)
    BINOP(||)
    BINOP(^)
    BINOP(|)
    BINOP(&)
    BINOP(>>)
    BINOP(<<)

    BINOP(==)
    BINOP(!=)
    BINOP(>)
    BINOP(>=)
    BINOP(<=)
    BINOP(<)

    UNOP(-)
    UNOP(+)
    UNOP(~)
    UNOP(!)
};

#undef BINOP
#undef UNOP

template <auto c> requires is_enum<decltype(c)>
struct int_const<c> {
    using type = decltype(c);
    static constexpr auto value = c;

    constexpr operator auto() const {
        return c;
    }

    constexpr auto to_int() const {
        return int_c<underlying_type<decltype(c)>(c)>;
    }
};

using true_t = int_const<true>;
using false_t = int_const<false>;

static constexpr inline auto true_c  = int_c<true>;
static constexpr inline auto false_c = int_c<false>;

template <auto... Vs>
struct intset : int_const<Vs>... {
    constexpr intset() = default;
    template <auto V> requires (bool(int_c<V> == int_c<Vs>) || ...)
    constexpr intset(int_const<V>) {}
};

namespace int_const_literals {
    template <char... cs>
    constexpr auto operator"" _c() {
        constexpr auto c = parse_int_checked<cs...>();
        if constexpr (c <= limits<int>::max())
            return int_t_c<int, c>;
        else if constexpr (c <= limits<unsigned int>::max())
            return int_t_c<unsigned int, c>;
        else if constexpr (c <= limits<long int>::max())
            return int_t_c<long int, c>;
        else if constexpr (c <= limits<unsigned long int>::max())
            return int_t_c<unsigned long int, c>;
        else if constexpr (c <= limits<long long int>::max())
            return int_t_c<long long int, c>;
        else if constexpr (c <= limits<unsigned long long int>::max())
            return int_t_c<unsigned long long int, c>;

    }

    template <char... cs>
    constexpr auto operator"" _uc() {
        constexpr auto c = parse_int_checked<cs...>();
        if constexpr (c <= limits<unsigned int>::max())
            return int_t_c<unsigned int, c>;
        else if constexpr (c <= limits<unsigned long int>::max())
            return int_t_c<unsigned long int, c>;
        else if constexpr (c <= limits<unsigned long long int>::max())
            return int_t_c<unsigned long long int, c>;
    }

    template <char... cs>
    constexpr auto operator"" _ulc() {
        constexpr auto c = parse_int_checked<cs...>();
        if constexpr (c <= limits<unsigned long int>::max())
            return int_t_c<unsigned long int, c>;
        else if constexpr (c <= limits<unsigned long long int>::max())
            return int_t_c<unsigned long long int, c>;
    }

    template <char... cs>
    constexpr auto operator"" _ullc() {
        constexpr auto c = parse_int_checked<cs...>();
        if constexpr (c <= limits<unsigned long long int>::max())
            return int_t_c<unsigned long long int, c>;
    }

    template <char... cs>
    constexpr auto operator"" _lc() {
        constexpr auto c = parse_int_checked<cs...>();
        if constexpr (c <= limits<long int>::max())
            return int_t_c<long int, c>;
        else if constexpr (c <= limits<long long int>::max())
            return int_t_c<long long int, c>;
        else if constexpr (c <= limits<unsigned long int>::max())
            return int_t_c<unsigned long int, c>;
        else if constexpr (c <= limits<unsigned long long int>::max())
            return int_t_c<unsigned long long int, c>;
    }

    template <char... cs>
    constexpr auto operator"" _llc() {
        constexpr auto c = parse_int_checked<cs...>();
        if constexpr (c <= limits<long long int>::max())
            return int_t_c<long long int, c>;
        else if constexpr (c <= limits<unsigned long long int>::max())
            return int_t_c<unsigned long long int, c>;
    }

    template <char... cs>
    constexpr auto operator"" _zc() {
        constexpr auto c = parse_int_checked<cs...>();
        if constexpr (c <= limits<ssize_t>::max())
            return int_t_c<ssize_t, c>;
        else if constexpr (c <= limits<size_t>::max())
            return int_t_c<size_t, c>;
    }

    template <char... cs>
    constexpr auto operator"" _zuc() {
        constexpr auto c = parse_int_checked<cs...>();
        if constexpr (c <= limits<size_t>::max())
            return int_t_c<size_t, c>;
    }
} // namespace int_const_literals
}// namespace core
