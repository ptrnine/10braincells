#pragma once

#include "../concepts/inheritance.hpp"
#include "../int_const.hpp"
#include "../traits/type_at_idx.hpp"
#include "../utility/forward.hpp"
#include "../utility/idx_dispatch.hpp"
#include "../utility/int_seq.hpp"
#include "../utility/move.hpp"
#include "type.hpp"

namespace core {
namespace details {
    template <typename Type, typename F>
    struct type_list_reduce_helper {
        constexpr type_list_reduce_helper(type_t<Type>, F func): f(mov(func)) {}
        F f;
    };

    template <typename V, typename T, typename F>
    constexpr auto operator+(V&& v, type_list_reduce_helper<T, F>&& f) {
        return f.f(forward<V>(v), type<T>);
    }
}

template <size_t I, typename T>
struct indexed_type {
    static inline constexpr size_t idx = I;
    using type = T;
};

template <typename... Ts>
struct type_list_t {
    template <typename T>
    constexpr auto prepend(T v) const {
        return type_prepend(*this, v);
    }

    template <typename T>
    constexpr auto append(T v) const {
        return type_append(*this, v);
    }

    constexpr auto pop_front() const {
        return type_pop_front(*this);
    }

    constexpr auto rotate_r() const {
        return type_rotate_r(*this);
    }

    constexpr auto rotate_l() const {
        return type_rotate_l(*this);
    }

    constexpr auto dedup() const {
        return type_dedup(*this);
    }

    template <typename... Ts2>
    constexpr bool operator==(type_list_t<Ts2...>) const {
        if constexpr (sizeof...(Ts) != sizeof...(Ts2))
            return false;
        else
            return ((type<Ts> == type<Ts2>) && ...) && true;
    }

    template <typename... Ts2>
    constexpr bool operator!=(type_list_t<Ts2...> tl) const {
        return !operator==(tl);
    }

    constexpr auto size() const {
        return sizeof...(Ts);
    }

    constexpr auto empty() const {
        return size() == 0;
    }

    template <auto v>
    constexpr auto operator[](int_const<v>) const {
        return type<type_at_idx<size_t(v), Ts...>>;
    }

    constexpr auto foreach(auto&& handler) const {
        (handler(type<Ts>), ...);
    }

    constexpr auto map(auto&& handler) const {
        return type_list_t<decltype(+handler(type<Ts>))...>{};
    }

    template <typename T, typename H>
    constexpr auto reduce(T&& init, H handler) const {
        return (forward<T>(init) + ... + details::type_list_reduce_helper{type<Ts>, handler});
    }

    constexpr auto dispatch(size_t i, auto&& function) const {
        return idx_dispatch<sizeof...(Ts)>(i, [&](auto idx) {
            return function(operator[](idx));
        });
    }

    constexpr auto indexed() const;
};

template <typename... Ts>
static inline constexpr type_list_t<Ts...> type_list = {};

namespace details {
    template <typename... Ts, size_t... Idxs>
    constexpr type_list_t<indexed_type<Idxs, Ts>...> make_indexed_helper(type_list_t<Ts...>, idx_seq<Idxs...>) {
        return {};
    }
}

template <typename... Ts>
constexpr auto make_indexed(type_list_t<Ts...> tl) {
    return details::make_indexed_helper(tl, make_idx_seq<sizeof...(Ts)>());
}

template <typename... Ts>
constexpr auto type_list_t<Ts...>::indexed() const {
    return make_indexed(*this);
}

template <typename T1, typename T2>
constexpr type_list_t<T1, T2> operator+(type_t<T1>, type_t<T2>) {
    return type_list<T1, T2>;
}

template <typename T, typename... Ts>
constexpr type_list_t<Ts..., T> operator+(type_list_t<Ts...>, type_t<T>) {
    return type_list<Ts..., T>;
}

template <typename T, typename... Ts>
constexpr type_list_t<T, Ts...> operator+(type_t<T>, type_list_t<Ts...>) {
    return type_list<T, Ts...>;
}

template <typename... Ts1, typename... Ts2>
constexpr type_list_t<Ts1..., Ts2...> operator+(type_list_t<Ts1...>, type_list_t<Ts2...>) {
    return type_list<Ts1..., Ts2...>;
}

template <typename... Ts, typename T>
constexpr auto type_prepend(type_list_t<Ts...> = {}, type_t<T> = {}) {
    return type_list<T, Ts...>;
}

template <typename... Ts, typename... Ts2>
constexpr auto type_prepend(type_list_t<Ts...> = {}, type_list_t<Ts2...> = {}) {
    return type_list<Ts2..., Ts...>;
}

template <typename... Ts, typename T>
constexpr auto type_append(type_list_t<Ts...> = {}, type_t<T> = {}) {
    return type_list<Ts..., T>;
}

template <typename... Ts, typename... Ts2>
constexpr auto type_append(type_list_t<Ts...> = {}, type_list_t<Ts2...> = {}) {
    return type_list<Ts..., Ts2...>;
}

template <typename T, typename... Ts>
constexpr auto type_pop_front(type_list_t<T, Ts...> = {}) {
    return type_list<Ts...>;
}

namespace type_details {
    template <typename T, typename... Rs>
    constexpr auto _rotate_r(type_list_t<T>, type_list_t<Rs...> = {}) {
        return type_list<T, Rs...>;
    }

    template <typename T1, typename T2, typename... Ts, typename... Rs>
    constexpr auto _rotate_r(type_list_t<T1, T2, Ts...>, type_list_t<Rs...> = {}) {
        return _rotate_r(type_list<T2, Ts...>, type_list<Rs..., T1>);
    }

    constexpr auto _rotate_r(type_list_t<>) {
        return type_list<>;
    }

    template <typename T, typename... Ts>
    constexpr auto _rotate_l(type_list_t<T, Ts...>) {
        return type_list<Ts..., T>;
    }

    constexpr auto _rotate_l(type_list_t<>) {
        return type_list<>;
    }
} // namespace type_details

template <typename... Ts>
constexpr auto type_rotate_r(type_list_t<Ts...> tl = {}) {
    return type_details::_rotate_r(tl);
}

template <typename... Ts>
constexpr auto type_rotate_l(type_list_t<Ts...> tl = {}) {
    return type_details::_rotate_l(tl);
}

namespace type_details {
    template <typename>
    struct dedup_test_holder {};

    template <typename... Ts>
    struct dedup_test : dedup_test_holder<Ts>... {};

    template <typename... Ts>
    constexpr auto _dedup(type_list_t<Ts...>, type_list_t<>) {
        return type_list<Ts...>;
    }

    template <typename... UniqTs, typename T, typename... Ts>
    constexpr auto _dedup(type_list_t<UniqTs...>, type_list_t<T, Ts...>) {
        if constexpr (base_of<dedup_test_holder<T>, dedup_test<UniqTs...>>)
            return _dedup(type_list<UniqTs...>, type_list<Ts...>);
        else
            return _dedup(type_list<UniqTs..., T>, type_list<Ts...>);
    }
}

template <typename... Ts>
constexpr auto type_dedup(type_list_t<Ts...> tl = {}) {
    return type_details::_dedup(type_list<>, tl);
}
} // namespace core
