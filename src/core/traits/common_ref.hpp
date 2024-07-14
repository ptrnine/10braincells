#pragma once

#include <core/concepts/convertible_to.hpp>
#include <core/traits/common_type.hpp>
#include <core/traits/copy_cvref.hpp>
#include <core/traits/declval.hpp>
#include <core/traits/is_ref.hpp>
#include <core/traits/remove_cvref.hpp>
#include <core/traits/remove_ref.hpp>

/* Stealed from llvm/libcxx */

namespace core
{
namespace details
{
    template <typename X, typename Y>
    using _cond_res = decltype(false ? declval<X (&)()>()() : declval<Y (&)()>()());

    template <typename T>
    struct _xref {
        template <typename U>
        using _apply = copy_cvref<T, U>;
    };

    template <typename A, typename B, typename X = remove_ref<A>, typename Y = remove_ref<B>>
    struct _common_ref;

    template <typename X, typename Y>
    using _common_ref_t = typename _common_ref<X, Y>::_type;

    template <typename X, typename Y>
    using _cv_cond_res = _cond_res<copy_cv<X, Y>&, copy_cv<Y, X>&>;

    template <typename A, typename B, typename X, typename Y>
        requires requires { typename _cv_cond_res<X, Y>; } && is_ref<_cv_cond_res<X, Y>>
    struct _common_ref<A&, B&, X, Y> {
        using _type = _cv_cond_res<X, Y>;
    };

    template <typename X, typename Y>
    using _common_ref_C = remove_ref<_common_ref_t<X&, Y&>>&&;

    template <typename A, typename B, typename X, typename Y>
        requires requires { typename _common_ref_C<X, Y>; } && convertible_to<A&&, _common_ref_C<X, Y>> &&
                 convertible_to<B&&, _common_ref_C<X, Y>>
    struct _common_ref<A&&, B&&, X, Y> {
        using _type = _common_ref_C<X, Y>;
    };

    template <typename T, typename U>
    using _common_ref_D = _common_ref_t<const T&, U&>;

    template <typename A, typename B, typename X, typename Y>
        requires requires { typename _common_ref_D<X, Y>; } && convertible_to<A&&, _common_ref_D<X, Y>>
    struct _common_ref<A&&, B&, X, Y> {
        using _type = _common_ref_D<X, Y>;
    };

    template <typename A, typename B, typename X, typename Y>
    struct _common_ref<A&, B&&, X, Y> : _common_ref<B&&, A&> {};

    template <typename A, typename B, typename X, typename Y>
    struct _common_ref {};
} // namespace details

template <typename...>
struct common_ref_s;

template <typename... _Types>
using common_ref = typename common_ref_s<_Types...>::type;

template <>
struct common_ref_s<> {};

template <typename T>
struct common_ref_s<T> {
  using type = T;
};

namespace details
{
  template <typename T, typename U>
  struct _common_ref_sub_bullet3;
  template <typename T, typename U>
  struct _common_ref_sub_bullet2 : _common_ref_sub_bullet3<T, U> {};
  template <typename T, typename U>
  struct _common_ref_sub_bullet1 : _common_ref_sub_bullet2<T, U> {};
} // namespace details

template <typename T, typename U>
struct common_ref_s<T, U> : details::_common_ref_sub_bullet1<T, U> {};

namespace details
{
  template <typename T, typename U>
      requires is_ref<T> && is_ref<U> && requires { typename _common_ref_t<T, U>; }
  struct _common_ref_sub_bullet1<T, U> {
        using type = _common_ref_t<T, U>;
  };
} // namespace details

template <typename, typename, template <typename> typename, template <typename> typename>
struct basic_common_ref {};

namespace details
{
  template <typename T, typename U>
  using _basic_common_ref_t = typename basic_common_ref<remove_cvref<T>,
                                                        remove_cvref<U>,
                                                        _xref<T>::template _apply,
                                                        _xref<U>::template _apply>::type;

  template <typename T, typename U>
      requires requires { typename _basic_common_ref_t<T, U>; }
  struct _common_ref_sub_bullet2<T, U> {
        using type = _basic_common_ref_t<T, U>;
  };

  template <typename T, typename U>
      requires requires { typename _cond_res<T, U>; }
  struct _common_ref_sub_bullet3<T, U> {
        using type = _cond_res<T, U>;
  };

  template <typename T, typename U>
  struct _common_ref_sub_bullet3 : common_type<T, U> {};
} // namespace details

template <typename T, typename U, typename V, typename... Ts>
  requires requires { typename common_ref<T, U>; }
struct common_ref_s<T, U, V, Ts...> : common_ref_s<common_ref<T, U>, V, Ts...> {};

template <typename...>
struct common_ref_s {};

template <typename... Ts>
concept have_common_ref = requires { typename common_ref_s<Ts...>::type; };
} // namespace core
