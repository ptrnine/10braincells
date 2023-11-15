#pragma once

#include <type_traits>

namespace core::details {
template <typename T>
constexpr bool check_enable(auto... settings) {
    return ((std::is_same_v<decltype(settings), T> ? int(T::enable) == int(settings) : false) || ...);
}

template <typename T>
constexpr auto settings_get_value(auto... settings) {
    constexpr auto get_v = [](auto v) constexpr {
        if constexpr (std::is_same_v<T, decltype(v)>)
            return +v;
        else
            return 0;
    };
    return (get_v(settings) + ... + 0);
}

struct autocast {
    template <typename T>
    operator T() {
        return {};
    }
};
} // namespace core::details
