#pragma once

#include <core/basic_types.hpp>

namespace core
{
namespace details
{
    struct fields_count_any {
        template <typename T>
        constexpr operator T() const;
    };

    template <size_t S, size_t C, typename T, typename... Ts>
    constexpr auto fields_count_f() {
        if constexpr (sizeof...(Ts) == S)
            return size_t(-1);
        else {
            if constexpr (requires { T{Ts{}...}; })
                return fields_count_f<S, C + 1, T, Ts..., fields_count_any>();
            else
                return C - 1;
        }
    }
} // namespace details

template <typename T>
static inline constexpr auto fields_count = details::fields_count_f<sizeof(T) + 2, 0, T>();
} // namespace core
