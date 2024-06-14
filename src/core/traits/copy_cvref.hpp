#pragma once

#include <core/traits/add_ref.hpp>
#include <core/traits/copy_cv.hpp>

namespace core
{
namespace details
{
    template <typename From, typename To>
    struct _copy_cvref {
        using type = copy_cv<From, To>;
    };
    template <typename From, typename To>
    struct _copy_cvref<From&, To> {
        using type = add_lvalue_ref<copy_cv<From, To>>;
    };
    template <typename From, typename To>
    struct _copy_cvref<From&&, To> {
        using type = add_rvalue_ref<copy_cv<From, To>>;
    };
} // namespace details
template <typename From, typename To>
using copy_cvref = typename details::_copy_cvref<From, To>::type;
} // namespace core
