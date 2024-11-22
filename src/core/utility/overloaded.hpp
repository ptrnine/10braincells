#pragma once

namespace core
{
template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

template <typename... Ts>
static inline constexpr auto overload_default = overloaded<decltype([](Ts) {})...>{};

template <typename T, typename... Ts>
static inline constexpr auto overload_default_r = overloaded<decltype([](Ts) {
    return T{};
})...>{};
} // namespace core
