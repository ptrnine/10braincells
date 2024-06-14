#pragma once

namespace core
{
namespace details
{
    template <typename From, typename To>
    struct _copy_cv {
        using type = To;
    };

    template <typename From, typename To>
    struct _copy_cv<const From, To> {
        using type = const To;
    };

    template <typename From, typename To>
    struct _copy_cv<volatile From, To> {
        using type = volatile To;
    };

    template <typename From, typename To>
    struct _copy_cv<const volatile From, To> {
        using type = const volatile To;
    };
} // namespace details

template <typename From, typename To>
using copy_cv = details::_copy_cv<From, To>::type;
} // namespace core
