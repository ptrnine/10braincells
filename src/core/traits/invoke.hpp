#pragma once

#include <core/traits/declval.hpp>

namespace core
{
namespace details
{
    template <typename F, typename... Ts>
    struct invoke_result_s {
        using type = decltype(declval<F>()(declval<Ts>()...));
    };

    template <typename R, typename Class, typename... Args, typename ClassArg, typename... Ts>
    struct invoke_result_s<R(Class::*)(Args...), ClassArg, Ts...> {
        using type = decltype((declval<ClassArg>().*declval<R(Class::*)(Args...)>())(declval<Ts>()...));
    };
    template <typename R, typename Class, typename... Args, typename ClassArg, typename... Ts>
    struct invoke_result_s<R(Class::*)(Args...) const, ClassArg, Ts...> {
        using type = decltype((declval<ClassArg>().*declval<R(Class::*)(Args...) const>())(declval<Ts>()...));
    };
    template <typename R, typename Class, typename... Args, typename ClassArg, typename... Ts>
    struct invoke_result_s<R(Class::*)(Args...) &, ClassArg, Ts...> {
        using type = decltype((declval<ClassArg>().*declval<R(Class::*)(Args...) &>())(declval<Ts>()...));
    };
    template <typename R, typename Class, typename... Args, typename ClassArg, typename... Ts>
    struct invoke_result_s<R(Class::*)(Args...) const&, ClassArg, Ts...> {
        using type = decltype((declval<ClassArg>().*declval<R(Class::*)(Args...) const&>())(declval<Ts>()...));
    };
    template <typename R, typename Class, typename... Args, typename ClassArg, typename... Ts>
    struct invoke_result_s<R(Class::*)(Args...) &&, ClassArg, Ts...> {
        using type = decltype((declval<ClassArg>().*declval<R(Class::*)(Args...) &&>())(declval<Ts>()...));
    };
    template <typename R, typename Class, typename... Args, typename ClassArg, typename... Ts>
    struct invoke_result_s<R (Class::*)(Args...) const&&, ClassArg, Ts...> {
        using type = decltype((declval<ClassArg>().*declval<R (Class::*)(Args...) const&&>())(declval<Ts>()...));
    };
    template <typename R, typename Class, typename... Args, typename ClassArg, typename... Ts>
    struct invoke_result_s<R(Class::*)(Args...) noexcept, ClassArg, Ts...> {
        using type = decltype((declval<ClassArg>().*declval<R(Class::*)(Args...) noexcept>())(declval<Ts>()...));
    };
    template <typename R, typename Class, typename... Args, typename ClassArg, typename... Ts>
    struct invoke_result_s<R(Class::*)(Args...) const noexcept, ClassArg, Ts...> {
        using type = decltype((declval<ClassArg>().*declval<R(Class::*)(Args...) const noexcept>())(declval<Ts>()...));
    };
    template <typename R, typename Class, typename... Args, typename ClassArg, typename... Ts>
    struct invoke_result_s<R(Class::*)(Args...) & noexcept, ClassArg, Ts...> {
        using type = decltype((declval<ClassArg>().*declval<R(Class::*)(Args...) & noexcept>())(declval<Ts>()...));
    };
    template <typename R, typename Class, typename... Args, typename ClassArg, typename... Ts>
    struct invoke_result_s<R(Class::*)(Args...) const& noexcept, ClassArg, Ts...> {
        using type = decltype((declval<ClassArg>().*declval<R(Class::*)(Args...) const& noexcept>())(declval<Ts>()...));
    };
    template <typename R, typename Class, typename... Args, typename ClassArg, typename... Ts>
    struct invoke_result_s<R(Class::*)(Args...) && noexcept, ClassArg, Ts...> {
        using type = decltype((declval<ClassArg>().*declval<R(Class::*)(Args...) && noexcept>())(declval<Ts>()...));
    };
    template <typename R, typename Class, typename... Args, typename ClassArg, typename... Ts>
    struct invoke_result_s<R (Class::*)(Args...) const&& noexcept, ClassArg, Ts...> {
        using type = decltype((declval<ClassArg>().*declval<R (Class::*)(Args...) const&& noexcept>())(declval<Ts>()...));
    };
}

template <typename F, typename... Ts>
using invoke_result = typename details::invoke_result_s<F, Ts...>::type;
} // namespace core
