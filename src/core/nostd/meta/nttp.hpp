#pragma once

namespace core {
template <auto value>
struct nttp_t {
    using type = decltype(value);

    constexpr auto operator+() const {
        return value;
    }
    constexpr operator auto() const {
        return value;
    }
};

template <auto value>
static inline constexpr nttp_t<value> nttp = {};
} // namespace core
