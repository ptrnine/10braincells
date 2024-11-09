#pragma once

#include <core/io/rw_impl_buff_ref.hpp>
#include <core/io/basic_types.hpp>
#include <core/io/file.hpp>
#include <core/meta/type.hpp>
#include <core/traits/conditional.hpp>
#include <core/traits/is_ref.hpp>

namespace core::io::details
{
template <typename T, bool read_only>
constexpr decltype(auto) deduce_backend_type() {
    using RT = remove_cvref<T>;
    if constexpr (is_same<RT, fd_t>) {
        return type<RT>;
    }
    else if constexpr (is_same<RT, file>) {
        if constexpr (is_rref<T>)
            return type<remove_rref<T>>;
        else
            return type<sys::fd_t>;
    }
    else {
        if constexpr (is_rref<T>)
            return type<remove_rref<RT>>;
        else
            return type<rw_impl_buff_ref<conditional<read_only, const RT, RT>>>;
    }
}
} // namespace core::io::details
