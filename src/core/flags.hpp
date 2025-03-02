#pragma once

#include <core/traits/is_enum.hpp>
#include <core/traits/underlying_type.hpp>

namespace core {
struct flags_using_enum {};

template <typename EnumT, typename UsingEnum = flags_using_enum>
    requires is_enum<EnumT>
struct flags : UsingEnum {
    using T = underlying_type<EnumT>;

    constexpr flags() = default;
    constexpr flags(EnumT flag): value(T(flag)) {}

    constexpr flags& operator|=(flags f) {
        value = T(value | f.value);
        return *this;
    }

    constexpr flags operator|(flags f) const {
        auto res = *this;
        res.value = T(value | f.value);
        return res;
    }

    constexpr flags& operator&=(flags f) {
        value = T(value | f.value);
        return *this;
    }

    constexpr flags operator&(flags f) const {
        auto res = *this;
        res.value = T(value | f.value);
        return res;
    }

    explicit constexpr operator bool() const {
        return value;
    }

    constexpr bool test(flags f) const {
        return value & f.value;
    }

    T value;
};
} // namespace core
