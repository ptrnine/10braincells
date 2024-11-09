#pragma once

#include <core/io/out_base.hpp>
#include <core/io/deduce_backend_type.hpp>

#define fwd(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace core::io
{
template <typename Backend, size_t BS = 8192>
class out : public out_base<Backend, BS> {
public:
    using out_tag = void;

    using out_base<Backend, BS>::out_base;
    using out_base<Backend, BS>::operator=;

    out(auto&& backend, int_const<BS> = {}): out_base<Backend, BS>(fwd(backend)) {}

    template <typename B>
    constexpr out(type_t<B>, int_const<BS> = {}) {}
};

template <typename B>
out(B&&) -> out<decl_type<details::deduce_backend_type<B&&, false>()>>;

template <typename B, size_t BS>
out(B&&, int_const<BS>) -> out<decl_type<details::deduce_backend_type<B&&, false>()>, BS>;

template <typename B>
out(type_t<B>) -> out<decl_type<details::deduce_backend_type<B&&, true>()>>;

template <typename B, size_t BS>
out(type_t<B>, int_const<BS>) -> out<decl_type<details::deduce_backend_type<B&&, true>()>, BS>;

template <typename T>
concept out_constraint = requires { typename remove_cvref<T>::out_tag; };
} // namespace core::io

#undef fwd
