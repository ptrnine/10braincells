#pragma once

#include "basic_types.hpp"
#include "ebo.hpp"
#include "int_const.hpp"
#include "utility/int_seq.hpp"
#include "traits/decay.hpp"
#include "traits/declval.hpp"
#include "utility/move.hpp"

#define fwd(what) static_cast<decltype(what)>(what)

namespace core {
namespace dtls {
    template <typename Idxs, typename... Ts>
    struct tuple_impl;

    template <size_t... Idxs, typename... Ts>
    struct tuple_impl<idx_seq<Idxs...>, Ts...> : ebo<Idxs, Ts>... {};
} // namespace dtls

template <typename... Ts>
struct tuple;

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
    return tuple<remove_rref<decltype(args)>...>{fwd(args)...};
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
    details::tuple_foreach(make_idx_seq<sizeof...(Ts)>(), move(*this), fwd(handler));
}

/* Map */
namespace details {
    template <size_t... Idxs>
    constexpr auto tuple_map(idx_seq<Idxs...>, auto&& tpl, auto&& handler) {
        return core::forward_as_tuple(handler(get<Idxs>(fwd(tpl)))...);
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
    return details::tuple_map(make_idx_seq<sizeof...(Ts)>(), move(*this), fwd(handler));
}

/* Reduce */
namespace details {
    template <typename T, typename F>
    struct tuple_reduce_helper {
        tuple_reduce_helper(T ivalue, F func): value(ivalue), handler(move(func)) {}
        T value;
        F handler;
    };

    template <typename T, typename F>
    auto operator+(auto&& initial, tuple_reduce_helper<T, F>&& h) {
        return h.handler(fwd(initial), static_cast<T>(h.value));
    }

    template <size_t... Idxs>
    constexpr auto tuple_reduce(idx_seq<Idxs...>, auto&& tpl, auto&& initial, auto&& handler) {
        return (fwd(initial) + ... +
                tuple_reduce_helper<decltype(get<Idxs>(fwd(tpl))), decltype(handler)>(get<Idxs>(fwd(tpl)), handler));
    }
}

template <typename... Ts>
constexpr auto tuple<Ts...>::reduce(auto&& initial, auto&& handler) const& {
    return details::tuple_reduce(make_idx_seq<sizeof...(Ts)>(), *this, fwd(initial), fwd(handler));
}

template <typename... Ts>
constexpr auto tuple<Ts...>::reduce(auto&& initial, auto&& handler)& {
    return details::tuple_reduce(make_idx_seq<sizeof...(Ts)>(), *this, fwd(initial), fwd(handler));
}

template <typename... Ts>
constexpr auto tuple<Ts...>::reduce(auto&& initial, auto&& handler) && {
    return details::tuple_reduce(make_idx_seq<sizeof...(Ts)>(), move(*this), fwd(initial), fwd(handler));
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
    using type = decltype(core::get<I>(declval<core::tuple<Ts...>>()));
};
} // namespace std

#undef fwd
