#pragma once

namespace core
{
#if __has_builtin(__remove_cvref)

template <typename T>
using remove_cvref = __remove_cvref(T);

#else
    #error "remove_cvref unimplemented"
#endif
} // namespace core
