#pragma once

#include <core/traits/is_const.hpp>
#include <core/traits/is_ref.hpp>
#include <core/traits/remove_ref.hpp>

#define fwd(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace core
{
auto bind_member(auto&& it, auto member_pointer) {
    if constexpr (is_rref<decltype(it)> && !is_const<remove_ref<decltype(it)>>)
        return [p = fwd(it), member_pointer](auto&&... args) mutable {
            return (p.*(member_pointer))(fwd(args)...);
        };
    else
        return [p = &it, member_pointer](auto&&... args) {
            return (p->*(member_pointer))(fwd(args)...);
        };
}
} // namespace core

#undef fwd
