#pragma once

#include <core/int_const.hpp>
#include <core/traits/is_same.hpp>

namespace core {
template <auto v, auto... vs>
struct accept_var {
    template <auto V>
        requires(bool(int_c<V>.to_int() == int_c<v>.to_int()) || (bool(int_c<V>.to_int() == int_c<vs>.to_int()) || ...))
    accept_var(int_const<V> val): value(val) {}

    decltype(v) value;
};

template <auto min, auto max>
struct accept_range {
    template <auto v>
        requires(bool(int_c<v>.to_int() >= int_c<min>.to_int()) && bool(int_c<v>.to_int() < int_c<max>.to_int()))
    accept_range(int_const<v> val): value(val.to_int()) {}

    decltype(min) value;
};
} // namespace core
