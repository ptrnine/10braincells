#pragma once

#include "../concepts/referenceable.hpp"

namespace core {
namespace dtls {
    template <typename T>
    struct add_rvalue_ref {
        using type = T;
    };

    template <referenceable T>
    struct add_rvalue_ref<T> {
        using type = T&&;
    };

    template <typename T>
    struct add_lvalue_ref {
        using type = T;
    };

    template <referenceable T>
    struct add_lvalue_ref<T> {
        using type = T&;
    };
} // namespace dtls
template <typename T>
using add_rvalue_ref = typename dtls::add_rvalue_ref<T>::type;
template <typename T>
using add_lvalue_ref = typename dtls::add_lvalue_ref<T>::type;
} // namespace core
