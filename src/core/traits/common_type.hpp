#pragma once

#include <core/traits/declval.hpp>
#include <core/traits/void_t.hpp>
#include <core/traits/remove_cvref.hpp>
#include <core/traits/decay.hpp>
#include <core/traits/is_same.hpp>
#include <core/traits/conditional.hpp>

namespace core
{
namespace details
{
    template <typename T, typename U>
    using _ternary_op = decltype(false ? declval<T>() : declval<U>());

    template <typename T, typename U>
    concept _ternary_op_ok = requires { typename _ternary_op<T, U>; };

    /* For types with deleted move ctor */
    template <typename T, typename U>
    struct _ct_no_mctor {};

    template <typename T, typename U> requires _ternary_op_ok<const T&, const U&>
    struct _ct_no_mctor<T, U> {
        using type = remove_cvref<_ternary_op<const T&, const U&>>;
    };

    /* For all types and void */
    template <typename T, typename U>
    struct _ct_base : _ct_no_mctor<T, U> {};

    template <typename T, typename U> requires _ternary_op_ok<T, U>
    struct _ct_base<T, U> {
        using type = decay<_ternary_op<T, U>>;
    };
} // namespace details

template <typename...>
struct common_type_s {};

template <typename... Ts>
concept have_common_type = requires { typename common_type_s<Ts...>::type; };

template <typename... Ts>
using common_type = common_type_s<Ts...>::type;

template <typename T>
struct common_type_s<T> : common_type_s<T, T> {};

template <typename T1, typename T2>
struct common_type_s<T1, T2> : conditional<is_same<T1, decay<T1>> && is_same<T2, decay<T2>>,
                                          details::_ct_base<T1, T2>,
                                          common_type_s<decay<T1>, decay<T2>>> {};

namespace details
{
    template <typename...>
    struct _ct_list;

    template <typename T>
    struct _ct_fold {};

    template <typename T, typename U> requires have_common_type<T, U>
    struct _ct_fold<_ct_list<T, U>> {
        using type = common_type<T, U>;
    };

    template <typename T, typename U, typename V, typename... Ts> requires have_common_type<T, U>
    struct _ct_fold<_ct_list<T, U, V, Ts...>> : _ct_fold<_ct_list<common_type<T, U>, V, Ts...>> {};
} // namespace details

template <typename T, typename U, typename V, typename... Ts>
struct common_type_s<T, U, V, Ts...> : details::_ct_fold<details::_ct_list<T, U, V, Ts...>> {};
} // namespace core
