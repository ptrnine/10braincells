#pragma once

#if __has_builtin(__remove_pointer)

namespace core
{
template <typename T>
using remove_ptr = __remove_pointer(T);
}

#else

namespace core
{
namespace details_traits
{
    template <typename T>
    struct _remove_ptr {
        using type = T;
    };
    template <typename T>
    struct _remove_ptr<T*> {
        using type = T;
    };
    template <typename T>
    struct _remove_ptr<const T*> {
        using type = T;
    };
    template <typename T>
    struct _remove_ptr<volatile T*> {
        using type = T;
    };
    template <typename T>
    struct _remove_ptr<const volatile T*> {
        using type = T;
    };
} // namespace details_traits

template <typename T>
using remove_ptr = typename details_traits::_remove_ptr<T>::type;
} // namespace core

#endif
