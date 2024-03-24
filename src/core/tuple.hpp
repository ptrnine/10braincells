#pragma once

#include "basic_types.hpp"
#include "ebo.hpp"
#include "int_const.hpp"
#include "meta/type_list.hpp"
#include "traits/decay.hpp"
#include "traits/declval.hpp"
#include "traits/type_at_idx.hpp"
#include "utility/int_seq.hpp"
#include "utility/move.hpp"

#define fwd(what) static_cast<decltype(what)>(what)

namespace core {
template <typename... Ts>
struct tuple;

namespace details
{
    template <typename T>
    struct is_tuple_impl {
        static inline constexpr bool value = false;
    };

    template <typename... Ts>
    struct is_tuple_impl<tuple<Ts...>> {
        static inline constexpr bool value = true;
    };
} // namespace details

template <typename T>
concept is_tuple = details::is_tuple_impl<T>::value;

template <typename T>
concept is_tuple_ref = details::is_tuple_impl<decay<T>>::value;

namespace dtls {
    template <typename Idxs, typename... Ts>
    struct tuple_impl;

    template <size_t... Idxs, typename... Ts>
    struct tuple_impl<idx_seq<Idxs...>, Ts...> : ebo<Idxs, Ts>... {
        constexpr auto forward() & {
            return tuple<remove_ref<Ts>&...>{get<Idxs>(*this)...};
        }
        constexpr auto forward() && {
            return tuple<Ts&&...>{get<Idxs>(mov(*this))...};
        }
        constexpr auto forward() const& {
            return tuple<const remove_ref<Ts>&...>{get<Idxs>(*this)...};
        }

        constexpr auto move() {
            return tuple<remove_ref<Ts>&&...>{mov(get<Idxs>(mov(*this)))...};
        }
        constexpr auto move() const {
            return tuple<const remove_ref<Ts>&&...>{mov(get<Idxs>(mov(*this)))...};
        }

        constexpr auto cat(is_tuple_ref auto&& value) &;
        constexpr auto cat(is_tuple_ref auto&& value) &&;
        constexpr auto cat(is_tuple_ref auto&& value) const&;

        constexpr auto pass_to(auto&& function) & {
            return fwd(function)(get<Idxs>(*this)...);
        }

        constexpr auto pass_to(auto&& function) && {
            return fwd(function)(get<Idxs>(mov(*this))...);
        }

        constexpr auto pass_to(auto&& function) const& {
            return fwd(function)(get<Idxs>(*this)...);
        }
    };

    template <typename T1, typename T2>
    struct tuple_cat_impl;

    template <size_t... Idxs1, typename... Ts1, size_t... Idxs2, typename... Ts2>
    struct tuple_cat_impl<tuple_impl<idx_seq<Idxs1...>, Ts1...>, tuple_impl<idx_seq<Idxs2...>, Ts2...>> {
        constexpr auto construct(auto&& t1, auto&& t2) {
            return tuple<Ts1..., Ts2...>{get<Idxs1>(fwd(t1))..., get<Idxs2>(fwd(t2))...};
        }
    };

    constexpr auto tuple_cat(auto&& t1, auto&& t2) {
        return tuple_cat_impl<decay<decltype(t1)>, typename decay<decltype(t2)>::super>{}.construct(fwd(t1), fwd(t2));
    }

    template <size_t... Idxs, typename... Ts>
    constexpr auto tuple_impl<idx_seq<Idxs...>, Ts...>::cat(is_tuple_ref auto&& value) & {
        return tuple_cat(*this, fwd(value));
    }

    template <size_t... Idxs, typename... Ts>
    constexpr auto tuple_impl<idx_seq<Idxs...>, Ts...>::cat(is_tuple_ref auto&& value) && {
        return tuple_cat(mov(*this), fwd(value));
    }

    template <size_t... Idxs, typename... Ts>
    constexpr auto tuple_impl<idx_seq<Idxs...>, Ts...>::cat(is_tuple_ref auto&& value) const& {
        return tuple_cat(*this, fwd(value));
    }
} // namespace dtls

template <typename... Ts>
struct tuple : dtls::tuple_impl<make_idx_seq<sizeof...(Ts)>, Ts...> {
    using super = dtls::tuple_impl<make_idx_seq<sizeof...(Ts)>, Ts...>;

    template <auto idx>
    constexpr decltype(auto) operator[](int_const<idx>) noexcept {
        return get<size_t(idx)>(*this);
    }

    template <auto idx>
    constexpr decltype(auto) operator[](int_const<idx>) const noexcept {
        return get<size_t(idx)>(*this);
    }

    static inline constexpr auto size() noexcept {
        return sizeof...(Ts);
    }

    static inline constexpr bool empty() noexcept {
        return size() == 0;
    }

    constexpr void foreach(auto&& handler) &;
    constexpr void foreach(auto&& handler) &&;
    constexpr void foreach(auto&& handler) const&;

    constexpr auto map(auto&& handler) &;
    constexpr auto map(auto&& handler) &&;
    constexpr auto map(auto&& handler) const&;

    constexpr auto reduce(auto&& initial, auto&& handler) &;
    constexpr auto reduce(auto&& initial, auto&& handler) &&;
    constexpr auto reduce(auto&& initial, auto&& handler) const&;
};

template <typename... Ts>
tuple(Ts...) -> tuple<decay<Ts>...>;

template <typename... Ts>
auto forward_as_tuple(Ts&&... args) {
    return tuple<decltype(args)...>{fwd(args)...};
}

template <typename... Ts>
auto fwd_as_tuple(Ts&&... args) {
    return tuple<decltype(args)...>{fwd(args)...};
}

/* Foreach */
namespace details {
    template <size_t... Idxs>
    constexpr void tuple_foreach(idx_seq<Idxs...>, auto&& tpl, auto&& handler) {
        (handler(get<Idxs>(fwd(tpl))), ...);
    }
}

template <typename... Ts>
constexpr void tuple<Ts...>::foreach(auto&& handler) const& {
    details::tuple_foreach(make_idx_seq<sizeof...(Ts)>(), *this, fwd(handler));
}

template <typename... Ts>
constexpr void tuple<Ts...>::foreach(auto&& handler) & {
    details::tuple_foreach(make_idx_seq<sizeof...(Ts)>(), *this, fwd(handler));
}

template <typename... Ts>
constexpr void tuple<Ts...>::foreach(auto&& handler) && {
    details::tuple_foreach(make_idx_seq<sizeof...(Ts)>(), mov(*this), fwd(handler));
}

/* Map */
namespace details {
    template <size_t... Idxs>
    constexpr auto tuple_map(idx_seq<Idxs...>, auto&& tpl, auto&& handler) {
        return tuple<decltype(handler(get<Idxs>(fwd(tpl))))...>{handler(get<Idxs>(fwd(tpl)))...};
    }
}

template <typename... Ts>
constexpr auto tuple<Ts...>::map(auto&& handler) const& {
    return details::tuple_map(make_idx_seq<sizeof...(Ts)>(), *this, fwd(handler));
}

template <typename... Ts>
constexpr auto tuple<Ts...>::map(auto&& handler) & {
    return details::tuple_map(make_idx_seq<sizeof...(Ts)>(), *this, fwd(handler));
}

template <typename... Ts>
constexpr auto tuple<Ts...>::map(auto&& handler) && {
    return details::tuple_map(make_idx_seq<sizeof...(Ts)>(), mov(*this), fwd(handler));
}

/* Reduce */
namespace details {
    template <typename T, typename F>
    struct tuple_reduce_helper {
        constexpr tuple_reduce_helper(T ivalue, F func): value(static_cast<T>(ivalue)), handler(func) {}
        T value;
        F handler;
    };

    template <typename T, typename F>
    constexpr auto operator+(auto&& initial, tuple_reduce_helper<T, F>&& h) {
        return h.handler(fwd(initial), static_cast<T>(h.value));
    }

    template <size_t... Idxs>
    constexpr auto tuple_reduce(idx_seq<Idxs...>, auto&& tpl, auto&& initial, auto&& handler) {
        return (fwd(initial) + ... +
                tuple_reduce_helper<decltype(get<Idxs>(fwd(tpl)))&&, decltype(handler)>(get<Idxs>(fwd(tpl)), handler));
    }
}

template <typename... Ts>
constexpr auto tuple<Ts...>::reduce(auto&& initial, auto&& handler) const& {
    return details::tuple_reduce(make_idx_seq<sizeof...(Ts)>(), *this, fwd(initial), handler);
}

template <typename... Ts>
constexpr auto tuple<Ts...>::reduce(auto&& initial, auto&& handler)& {
    return details::tuple_reduce(make_idx_seq<sizeof...(Ts)>(), *this, fwd(initial), handler);
}

template <typename... Ts>
constexpr auto tuple<Ts...>::reduce(auto&& initial, auto&& handler) && {
    return details::tuple_reduce(make_idx_seq<sizeof...(Ts)>(), mov(*this), fwd(initial), handler);
}
} // namespace core


namespace std {
template <typename>
struct tuple_size;

template <typename... Ts>
struct tuple_size<core::tuple<Ts...>> {
    static inline constexpr size_t value = sizeof...(Ts);
};

template <size_t, typename>
struct tuple_element;

template <size_t I, typename... Ts>
struct tuple_element<I, core::tuple<Ts...>> {
    using type = core::type_at_idx<I, Ts...>;
};
} // namespace std

#undef fwd
