#pragma once

#if !__has_builtin(__is_convertible)
#include "../traits/declval.hpp"
#include "../traits/is_array.hpp"
#include "../traits/is_function.hpp"
#endif

namespace core {
namespace details {
#if __has_builtin(__is_convertible)
    template <typename From, typename To>
    static constexpr bool _is_convertible = __is_convertible(From, To);
#else
    template <typename From, typename To>
    struct _is_convertible_helper {
        static inline constexpr bool value = false;
    };

    template <>
    struct _is_convertible_helper<void, void> {
        static inline constexpr bool value = true;
    };

    template <typename T>
    concept array_or_function = is_array<T> || is_function<T>;

    template <typename From, array_or_function To>
    struct _is_convertible_helper<From, To> {
        static inline constexpr bool value = false;
    };

    template <typename T>
    void _test_convert(T);

    template <typename From, typename To> 
        requires (!array_or_function<To>) && requires { _test_convert<To>(declval<From>()); }
    struct _is_convertible_helper<From, To> {
        static inline constexpr bool value = true;
    };

    template <typename From, typename To>
    inline constexpr bool _is_convertible = _is_convertible_helper<From, To>::value;
#endif
}

template <typename From, typename To>
concept convertible_to = details::_is_convertible<From, To>;
} // namespace core
