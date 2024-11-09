#pragma once

#include <core/io/in_base.hpp>
#include <core/io/deduce_backend_type.hpp>

#define fwd(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace core::io
{
template <typename Backend, size_t BS = 8192>
class in : public in_base<Backend, BS> {
public:
    using in_tag = void;

    using in_base<Backend, BS>::in_base;
    using in_base<Backend, BS>::operator=;

    constexpr in(auto&& backend, int_const<BS> = {}): in_base<Backend, BS>(fwd(backend)) {}

    template <typename B>
    constexpr in(type_t<B>, int_const<BS> = {}) {}
};

template <typename B>
in(B&&) -> in<decl_type<details::deduce_backend_type<B&&, true>()>>;

template <typename B, size_t BS>
in(B&&, int_const<BS>) -> in<decl_type<details::deduce_backend_type<B&&, true>()>, BS>;

template <typename B>
in(type_t<B>) -> in<decl_type<details::deduce_backend_type<B&&, true>()>>;

template <typename B, size_t BS>
in(type_t<B>, int_const<BS>) -> in<decl_type<details::deduce_backend_type<B&&, true>()>, BS>;

template <typename T>
concept in_constraint = requires { typename remove_cvref<T>::in_tag; };
#undef fwd
} // namespace core::io

#undef fwd
