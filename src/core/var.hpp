#pragma once

#include "concepts/assign.hpp"
#include "concepts/ctor.hpp"
#include "concepts/nothrow_assign.hpp"
#include "concepts/nothrow_ctor.hpp"
#include "concepts/trivial_assign.hpp"
#include "concepts/trivial_ctor.hpp"
#include "concepts/trivial_dtor.hpp"
#include "exception.hpp"
#include "int_const.hpp"
#include "meta/type_list.hpp"
#include "traits/declval.hpp"
#include "traits/idx_of_type.hpp"
#include "traits/type_at_idx.hpp"
#include "utility/idx_dispatch.hpp"
#include "utility/idx_dispatch_r.hpp"
#include "utility/move.hpp"
#include "utility/overloaded.hpp"

#define fwd(x) static_cast<decltype(x)>(x)

namespace core {
template <size_t S>
inline constexpr auto size_c = int_c<S>;

namespace dtls {
    template <size_t IdxS, typename...>
    union var_storage;

    template <size_t IdxS>
    union var_storage<IdxS> {};

    template <typename... Ts>
    concept all_triv_dtor = (trivial_dtor<Ts> && ...);


#define def_trivdtor_storage_field(i)                                               \
    T##i v##i;                                                                      \
    constexpr var_storage(int_const<IdxS+i>, auto&&... args): v##i(fwd(args)...) {} \
    constexpr void        emplace(int_const<IdxS+i> idx, auto&&... args) {          \
        if constexpr (trivial_move_assign<var_storage>)                             \
            *this = var_storage(idx, fwd(args)...);                                 \
        else                                                                        \
            ::new (&v##i) T##i(fwd(args)...);                                       \
    }                                                                               \
    constexpr T##i&       get(int_const<IdxS+i>) noexcept { return v##i; }          \
    constexpr const T##i& get(int_const<IdxS+i>) const noexcept { return v##i; }


#define def_storage_field(i)                                                                               \
    T##i v##i;                                                                                             \
    constexpr var_storage(int_const<IdxS+i>, auto&&... args): v##i(fwd(args)...) {}                        \
    constexpr void        emplace(int_const<IdxS+i>, auto&&... args) { ::new (&v##i) T##i(fwd(args)...); } \
    constexpr T##i&       get(int_const<IdxS+i>) noexcept { return v##i; }                                 \
    constexpr const T##i& get(int_const<IdxS+i>) const noexcept { return v##i; }

    /* 1 arg */
    template <size_t IdxS, typename T0>
        requires all_triv_dtor<T0>
    union var_storage<IdxS, T0> {
        def_trivdtor_storage_field(0)
    };

    template <size_t IdxS, typename T0>
    union var_storage<IdxS, T0> {
        constexpr ~var_storage() {}
        def_storage_field(0)
    };

    /* 2 args */
    template <size_t IdxS, typename T0, typename T1>
        requires all_triv_dtor<T0, T1>
    union var_storage<IdxS, T0, T1> {
        def_trivdtor_storage_field(0) def_trivdtor_storage_field(1)
    };

    template <size_t IdxS, typename T0, typename T1>
    union var_storage<IdxS, T0, T1> {
        constexpr ~var_storage() {}
        def_storage_field(0) def_storage_field(1)
    };

    /* 3 args */
    template <size_t IdxS, typename T0, typename T1, typename T2>
        requires all_triv_dtor<T0, T1, T2>
    union var_storage<IdxS, T0, T1, T2> {
        def_trivdtor_storage_field(0) def_trivdtor_storage_field(1) def_trivdtor_storage_field(2)
    };

    template <size_t IdxS, typename T0, typename T1, typename T2>
    union var_storage<IdxS, T0, T1, T2> {
        constexpr ~var_storage() {}
        def_storage_field(0) def_storage_field(1) def_storage_field(2)
    };

    /* 4 args */
    template <size_t IdxS, typename T0, typename T1, typename T2, typename T3>
        requires all_triv_dtor<T0, T1, T2, T3>
    union var_storage<IdxS, T0, T1, T2, T3> {
        def_trivdtor_storage_field(0) def_trivdtor_storage_field(1) def_trivdtor_storage_field(2)
        def_trivdtor_storage_field(3)
    };

    template <size_t IdxS, typename T0, typename T1, typename T2, typename T3>
    union var_storage<IdxS, T0, T1, T2, T3> {
        constexpr ~var_storage() {}
        def_storage_field(0) def_storage_field(1) def_storage_field(2) def_storage_field(3)
    };

    /* 5 args */
    template <size_t IdxS, typename T0, typename T1, typename T2, typename T3, typename T4>
        requires all_triv_dtor<T0, T1, T2, T3, T4>
    union var_storage<IdxS, T0, T1, T2, T3, T4> {
        def_trivdtor_storage_field(0) def_trivdtor_storage_field(1) def_trivdtor_storage_field(2)
        def_trivdtor_storage_field(3) def_trivdtor_storage_field(4)
    };

    template <size_t IdxS, typename T0, typename T1, typename T2, typename T3, typename T4>
    union var_storage<IdxS, T0, T1, T2, T3, T4> {
        constexpr ~var_storage() {}
        def_storage_field(0) def_storage_field(1) def_storage_field(2) def_storage_field(3) def_storage_field(4)
    };

    /* 6 args */
    template <size_t IdxS, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5>
        requires all_triv_dtor<T0, T1, T2, T3, T4, T5>
    union var_storage<IdxS, T0, T1, T2, T3, T4, T5> {
        def_trivdtor_storage_field(0) def_trivdtor_storage_field(1) def_trivdtor_storage_field(2)
        def_trivdtor_storage_field(3) def_trivdtor_storage_field(4) def_trivdtor_storage_field(5)
    };

    template <size_t IdxS, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5>
    union var_storage<IdxS, T0, T1, T2, T3, T4, T5> {
        constexpr ~var_storage() {}
        def_storage_field(0) def_storage_field(1) def_storage_field(2) def_storage_field(3) def_storage_field(4)
        def_storage_field(5)
    };

    /* 7 args */
    template <size_t IdxS, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
        requires all_triv_dtor<T0, T1, T2, T3, T4, T5, T6>
    union var_storage<IdxS, T0, T1, T2, T3, T4, T5, T6> {
        def_trivdtor_storage_field(0) def_trivdtor_storage_field(1) def_trivdtor_storage_field(2)
        def_trivdtor_storage_field(3) def_trivdtor_storage_field(4) def_trivdtor_storage_field(5)
        def_trivdtor_storage_field(6)
    };

    template <size_t IdxS, typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
    union var_storage<IdxS, T0, T1, T2, T3, T4, T5, T6> {
        constexpr ~var_storage() {}
        def_storage_field(0) def_storage_field(1) def_storage_field(2) def_storage_field(3) def_storage_field(4)
        def_storage_field(5) def_storage_field(6)
    };

    /* 8 args */
    template <size_t IdxS,
              typename T0,
              typename T1,
              typename T2,
              typename T3,
              typename T4,
              typename T5,
              typename T6,
              typename T7>
        requires all_triv_dtor<T0, T1, T2, T3, T4, T5, T6, T7>
    union var_storage<IdxS, T0, T1, T2, T3, T4, T5, T6, T7> {
        def_trivdtor_storage_field(0) def_trivdtor_storage_field(1) def_trivdtor_storage_field(2)
        def_trivdtor_storage_field(3) def_trivdtor_storage_field(4) def_trivdtor_storage_field(5)
        def_trivdtor_storage_field(6) def_trivdtor_storage_field(7)
    };

    template <size_t IdxS,
              typename T0,
              typename T1,
              typename T2,
              typename T3,
              typename T4,
              typename T5,
              typename T6,
              typename T7>
    union var_storage<IdxS, T0, T1, T2, T3, T4, T5, T6, T7> {
        constexpr ~var_storage() {}
        def_storage_field(0) def_storage_field(1) def_storage_field(2) def_storage_field(3) def_storage_field(4)
        def_storage_field(5) def_storage_field(6) def_storage_field(7)
    };

    /* 9 args */
    template <size_t IdxS,
              typename T0,
              typename T1,
              typename T2,
              typename T3,
              typename T4,
              typename T5,
              typename T6,
              typename T7,
              typename T8>
        requires all_triv_dtor<T0, T1, T2, T3, T4, T5, T6, T7, T8>
    union var_storage<IdxS, T0, T1, T2, T3, T4, T5, T6, T7, T8> {
        def_trivdtor_storage_field(0) def_trivdtor_storage_field(1) def_trivdtor_storage_field(2)
        def_trivdtor_storage_field(3) def_trivdtor_storage_field(4) def_trivdtor_storage_field(5)
        def_trivdtor_storage_field(6) def_trivdtor_storage_field(7) def_trivdtor_storage_field(8)
    };

    template <size_t IdxS,
              typename T0,
              typename T1,
              typename T2,
              typename T3,
              typename T4,
              typename T5,
              typename T6,
              typename T7,
              typename T8>
    union var_storage<IdxS, T0, T1, T2, T3, T4, T5, T6, T7, T8> {
        constexpr ~var_storage() {}
        def_storage_field(0) def_storage_field(1) def_storage_field(2) def_storage_field(3) def_storage_field(4)
        def_storage_field(5) def_storage_field(6) def_storage_field(7) def_storage_field(8)
    };

    /* 10 - N args */
    template <size_t IdxS,
              typename T0,
              typename T1,
              typename T2,
              typename T3,
              typename T4,
              typename T5,
              typename T6,
              typename T7,
              typename T8,
              typename T9,
              typename... Ts>
        requires all_triv_dtor<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, Ts...>
    union var_storage<IdxS, T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, Ts...> {
        def_trivdtor_storage_field(0) def_trivdtor_storage_field(1) def_trivdtor_storage_field(2)
        def_trivdtor_storage_field(3) def_trivdtor_storage_field(4) def_trivdtor_storage_field(5)
        def_trivdtor_storage_field(6) def_trivdtor_storage_field(7) def_trivdtor_storage_field(8)
        def_trivdtor_storage_field(9)

        var_storage<10, Ts...> vs;

        template <size_t I>
        constexpr var_storage(int_const<I> idx, auto&&... args): vs(idx - size_c<IdxS>, static_cast<decltype(args)>(args)...) {}
        template <size_t I>
        constexpr void emplace(int_const<I> idx, auto&&... args) {
            vs.emplace(idx - size_c<IdxS>, static_cast<decltype(args)>(args)...);
        }
        template <size_t I>
        constexpr decltype(auto) get(int_const<I> idx) noexcept {
            return vs.get(idx - size_c<IdxS>);
        }
        template <size_t I>
        constexpr decltype(auto) get(int_const<I> idx) const noexcept {
            return vs.get(idx - size_c<IdxS>);
        }
    };

    template <size_t IdxS,
              typename T0,
              typename T1,
              typename T2,
              typename T3,
              typename T4,
              typename T5,
              typename T6,
              typename T7,
              typename T8,
              typename T9,
              typename... Ts>
    union var_storage<IdxS, T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, Ts...> {
        constexpr ~var_storage() {}
        def_storage_field(0) def_storage_field(1) def_storage_field(2) def_storage_field(3) def_storage_field(4)
        def_storage_field(5) def_storage_field(6) def_storage_field(7) def_storage_field(8) def_storage_field(9)

        var_storage<10, Ts...> vs;

        template <size_t I>
        constexpr var_storage(int_const<I> idx, auto&&... args): vs(idx - size_c<IdxS>, static_cast<decltype(args)>(args)...) {}
        template <size_t I>
        constexpr void emplace(int_const<I> idx, auto&&... args) {
            vs.emplace(idx - size_c<IdxS>, static_cast<decltype(args)>(args)...);
        }
        template <size_t I>
        constexpr decltype(auto) get(int_const<I> idx) noexcept {
            return vs.get(idx - size_c<IdxS>);
        }
        template <size_t I>
        constexpr decltype(auto) get(int_const<I> idx) const noexcept {
            return vs.get(idx - size_c<IdxS>);
        }
    };


#undef def_trivdtor_storage_field
#undef def_storage_field

template <size_t S>
auto choose_idx_type() {
    /* TODO: bench */
    if constexpr (S <= u8(-1))
        return u8{};
    else if constexpr (S <= u16(-1))
        return u16{};
    else if constexpr (S <= u32(-1))
        return u32{};
    else if constexpr (sizeof(S) < sizeof(u64))
        return u64{};
}

template <size_t S>
using idx_type = decltype(choose_idx_type<S>());


/* non-trivially destructible
 * throw move constructible
 *
 * automatically call suitable destructor for stored value
 * use double-buffering to avoid valueless state
 */
struct var_none {};

template <typename... Ts>
struct var_base;

template <typename... Ts> requires (!nothrow_move_ctor<Ts> || ...) && (!trivial_dtor<Ts> || ...)
struct var_base<Ts...> {
    using i_type = idx_type<sizeof...(Ts)>;
    using storage_t = var_storage<0, var_none, Ts...>;

    storage_t _storage[2];
    struct {
        i_type _idx;
        u8     _bpos;
    } _b;

    static constexpr auto types_count() noexcept {
        return sizeof...(Ts);
    }

    constexpr var_base(): _storage{{size_c<0>}, {size_c<0>}}, _b{i_type(-1), 0} {}

    template <size_t I>
    constexpr explicit var_base(int_const<I> i, auto&&... args):
        _storage{{i + size_c<1>, fwd(args)...}, {size_c<0>}}, _b{i_type(i), 0} {}

    constexpr ~var_base() {
        _destroy();
    }

    template <size_t I>
    constexpr void _init(int_const<I> i, auto&&... args) {
        ::new(&_storage[0]) storage_t(i + size_c<1>, fwd(args)...);
        _b._idx = i_type(i);
    }

    constexpr void _destroy() noexcept {
        idx_dispatch<sizeof...(Ts)>(_b._idx, [this](auto i) {
            using T = type_at_idx_pack<i, type_list_t<Ts...>>;
            _storage[_b._bpos].get(i + size_c<1>).~T();
        });
    }

    constexpr size_t index() const noexcept {
        return _b._idx;
    }

    template <size_t I>
    constexpr void _emplace(int_const<I> i, auto&&... args) {
        /* TODO: optimize for nothrow ctor
         * it is not necessary to change storage in nothrow case
         *
         * XXX: Optimized, should be tested
         */
        if constexpr (nothrow_ctor<type_at_idx<i, Ts...>, decltype(args)...>) {
            _destroy();
            _storage[_b._bpos].emplace(i + size_c<1>, fwd(args)...);
            _b._idx = i_type(i);
        }
        else {
            auto newbpos = _b._bpos ^ 1;
            _storage[newbpos].emplace(i + size_c<1>, fwd(args)...);
            _destroy();
            _b._idx  = i_type(i);
            _b._bpos = u8(newbpos);
        }
    }

    template <size_t I>
    constexpr decltype(auto) _get(int_const<I> i) noexcept {
        return _storage[_b._bpos].get(i + size_c<1>);
    }

    template <size_t I>
    constexpr decltype(auto) _get(int_const<I> i) const noexcept {
        return _storage[_b._bpos].get(i + size_c<1>);
    }
};

/* trivially destructible
 * throw move constructible
 *
 * use double-buffering to avoid valueless state
 */
template <trivial_dtor... Ts> requires (!nothrow_move_ctor<Ts> || ...)
struct var_base<Ts...> {
    using i_type = idx_type<sizeof...(Ts)>;
    using storage_t = var_storage<0, var_none, Ts...>;

    storage_t _storage[2];
    struct {
        i_type _idx;
        u8     _bpos;
    } _b;

    static constexpr auto types_count() noexcept {
        return sizeof...(Ts);
    }

    constexpr var_base(): _storage{{size_c<0>}, {size_c<0>}}, _b{i_type(-1), 0} {}

    template <size_t I>
    constexpr explicit var_base(int_const<I> i, auto&&... args):
        _storage{{i + size_c<1>, fwd(args)...}, {size_c<0>}}, _b{i_type(i), 0} {}

    template <size_t I>
    constexpr void _init(int_const<I> i, auto&&... args) {
        ::new (&_storage[0]) storage_t(i + size_c<1>, fwd(args)...);
        _b._idx  = i_type(i);
    }

    constexpr size_t index() const noexcept {
        return _b._idx;
    }

    template <size_t I>
    constexpr void _emplace(int_const<I> i, auto&&... args) {
        /* TODO: optimize for nothrow ctor
         * it is not necessary to change storage in nothrow case
         *
         * XXX: Optimized, should be tested
         */
        if constexpr (nothrow_ctor<type_at_idx<i, Ts...>, decltype(args)...>) {
            _storage[_b._bpos].emplace(i + size_c<1>, fwd(args)...);
            _b._idx = i_type(i);
        }
        else {
            auto newpos = _b._bpos ^ 1;
            _storage[newpos].emplace(i + size_c<1>, fwd(args)...);
            _b._idx = i_type(i);
            _b._bpos = u8(newpos);
        }
    }

    template <size_t I>
    constexpr decltype(auto) _get(int_const<I> i) noexcept {
        return _storage[_b._bpos].get(i + size_c<1>);
    }

    template <size_t I>
    constexpr decltype(auto) _get(int_const<I> i) const noexcept {
        return _storage[_b._bpos].get(i + size_c<1>);
    }
};

/* non-trivially destructible
 * nothrow move constructible
 *
 * automatically call suitable destructor for stored value
 */
template <nothrow_move_ctor... Ts> requires (!trivial_dtor<Ts> || ...)
struct var_base<Ts...> {
    using i_type    = idx_type<sizeof...(Ts)>;
    using storage_t = var_storage<0, var_none, Ts...>;

    storage_t _storage;
    i_type    _idx;

    static constexpr auto types_count() noexcept {
        return sizeof...(Ts);
    }

    constexpr var_base(): _storage{size_c<0>}, _idx(-1) {}

    template <size_t I>
    constexpr explicit var_base(int_const<I> i, auto&&... args):
        _storage(i + size_c<1>, fwd(args)...), _idx(i) {}

    constexpr ~var_base() {
        _destroy();
    }

    template <size_t I>
    constexpr void _init(int_const<I> i, auto&&... args) {
        ::new(&_storage) storage_t(i + size_c<1>, fwd(args)...);
        _idx = i_type(i);
    }

    constexpr void _destroy() noexcept {
        idx_dispatch<sizeof...(Ts)>(_idx, [this](auto i) {
            using T = type_at_idx_pack<i, type_list_t<Ts...>>;
            _storage.get(i + size_c<1>).~T();
        });
    }

    constexpr size_t index() const noexcept {
        return _idx;
    }

    template <size_t I>
    constexpr void _emplace(int_const<I> i, auto&&... args) {
        using T = type_at_idx_pack<i, type_list_t<Ts...>>;
        if constexpr (nothrow_ctor<T, decltype(args)...>) {
            _destroy();
            _storage.emplace(i + size_c<1>, fwd(args)...);
        }
        else {
            T tmp(fwd(args)...);
            _destroy();
            _storage.emplace(i + size_c<1>, static_cast<T&&>(tmp));
        }
        _idx = i_type(i);
    }

    template <size_t I>
    constexpr decltype(auto) _get(int_const<I> i) noexcept {
        return _storage.get(i + size_c<1>);
    }

    template <size_t I>
    constexpr decltype(auto) _get(int_const<I> i) const noexcept {
        return _storage.get(i + size_c<1>);
    }
};

/* trivially destructible
 * nothrow move constructible
 */
template <typename... Ts> requires (trivial_dtor<Ts> && ...) && (nothrow_move_ctor<Ts> && ...)
struct var_base<Ts...> {
    using i_type    = idx_type<sizeof...(Ts)>;
    using storage_t = var_storage<0, var_none, Ts...>;

    storage_t _storage;
    i_type    _idx;

    static constexpr auto types_count() noexcept {
        return sizeof...(Ts);
    }

    constexpr var_base(): _storage{size_c<0>}, _idx(-1) {}

    template <size_t I>
    constexpr explicit var_base(int_const<I> i, auto&&... args):
        _storage(i + int_c<1>, fwd(args)...), _idx(i) {}

    template <size_t I>
    constexpr void _init(int_const<I> i, auto&&... args) {
        ::new(&_storage) storage_t(i + int_c<1>, fwd(args)...);
        _idx = i;
    }

    constexpr size_t index() const noexcept {
        return _idx;
    }

    template <size_t I>
    constexpr void _emplace(int_const<I> i, auto&&... args) {
         _storage.emplace(i + int_c<1>, fwd(args)...);
        _idx = i;
    }

    template <size_t I>
    constexpr decltype(auto) _get(int_const<I> i) noexcept {
        return _storage.get(i + int_c<1>);
    }

    template <size_t I>
    constexpr decltype(auto) _get(int_const<I> i) const noexcept {
        return _storage.get(i + int_c<1>);
    }
};

#define def_non_trivial(X) \
template <typename... Ts> \
concept nontrivial_##X##s = !(trivial_##X<Ts> && ...)

def_non_trivial(copy_ctor);
def_non_trivial(copy_assign);
def_non_trivial(move_ctor);
def_non_trivial(move_assign);

#undef def_non_trivial

template <typename D, typename...>
struct var_cctor : D {
    using D::D;
    constexpr var_cctor() noexcept                   = default;
    constexpr var_cctor(const var_cctor&)            = delete;
    constexpr var_cctor& operator=(const var_cctor&) = default;
    constexpr var_cctor(var_cctor&&)                 = default;
    constexpr var_cctor& operator=(var_cctor&&)      = default;
};


template <typename D, trivial_copy_ctor... Ts>
struct var_cctor<D, Ts...> : D {
    using D::D;
    constexpr var_cctor() noexcept                                                   = default;
    constexpr var_cctor(const var_cctor& r) noexcept((nothrow_copy_ctor<Ts> && ...)) = default;
    constexpr var_cctor& operator=(const var_cctor&)                                 = default;
    constexpr var_cctor(var_cctor&&)                                                 = default;
    constexpr var_cctor& operator=(var_cctor&&)                                      = default;
};

template <typename D, copy_ctor... Ts> requires nontrivial_copy_ctors<Ts...>
struct var_cctor<D, Ts...> : D {
    using D::D;
    constexpr var_cctor() noexcept = default;
    constexpr var_cctor(const var_cctor& r) noexcept((nothrow_copy_ctor<Ts> && ...)): D() {
        idx_dispatch<sizeof...(Ts)>(r.index(), [&](auto i) {
            this->_init(i, r._get(i));
        });
    }
    constexpr var_cctor& operator=(const var_cctor&) = default;
    constexpr var_cctor(var_cctor&&)                 = default;
    constexpr var_cctor& operator=(var_cctor&&)      = default;
};

template <typename D, typename...>
struct var_casgn : D {
    using D::D;
    constexpr var_casgn() noexcept                   = default;
    constexpr var_casgn(const var_casgn&)            = default;
    constexpr var_casgn& operator=(const var_casgn&) = delete;
    constexpr var_casgn(var_casgn&&)                 = default;
    constexpr var_casgn& operator=(var_casgn&&)      = default;
};

template <typename D, trivial_copy_assign... Ts>
struct var_casgn<D, Ts...> : D {
    using D::D;
    constexpr var_casgn() noexcept                                                                = default;
    constexpr var_casgn(const var_casgn&)                                                         = default;
    constexpr var_casgn& operator=(const var_casgn& r) noexcept((nothrow_copy_assign<Ts> && ...)) = default;
    constexpr var_casgn(var_casgn&&)                                                              = default;
    constexpr var_casgn& operator=(var_casgn&&)                                                   = default;
};

template <typename D, copy_assign... Ts> requires nontrivial_copy_assigns<Ts...>
struct var_casgn<D, Ts...> : D {
    using D::D;
    constexpr var_casgn() noexcept        = default;
    constexpr var_casgn(const var_casgn&) = default;
    constexpr var_casgn& operator=(const var_casgn& r) noexcept((nothrow_copy_assign<Ts> && ...)) {
        idx_dispatch<sizeof...(Ts)>(r.index(), [&](auto i) {
            this->_emplace(i, r._get(i));
        });
        return *this;
    }
    constexpr var_casgn(var_casgn&&)            = default;
    constexpr var_casgn& operator=(var_casgn&&) = default;
};

template <typename D, typename...>
struct var_mctor : D {
    using D::D;
    constexpr var_mctor() noexcept                   = default;
    constexpr var_mctor(const var_mctor&)            = default;
    constexpr var_mctor& operator=(const var_mctor&) = default;
    constexpr var_mctor(var_mctor&&)                 = delete;
    constexpr var_mctor& operator=(var_mctor&&)      = default;
};

template <typename D, trivial_move_ctor... Ts>
struct var_mctor<D, Ts...> : D {
    using D::D;
    constexpr var_mctor() noexcept                                              = default;
    constexpr var_mctor(const var_mctor&)                                       = default;
    constexpr var_mctor& operator=(const var_mctor&)                            = default;
    constexpr var_mctor(var_mctor&& r) noexcept((nothrow_move_ctor<Ts> && ...)) = default;
    constexpr var_mctor& operator=(var_mctor&&)                                 = default;
};

template <typename D, move_ctor... Ts> requires nontrivial_move_ctors<Ts...>
struct var_mctor<D, Ts...> : D {
    using D::D;
    constexpr var_mctor() noexcept                   = default;
    constexpr var_mctor(const var_mctor&)            = default;
    constexpr var_mctor& operator=(const var_mctor&) = default;
    constexpr var_mctor(var_mctor&& r) noexcept((nothrow_move_ctor<Ts> && ...)): D() {
        idx_dispatch<sizeof...(Ts)>(r.index(), [&](auto i) {
            using type = decltype(auto(r._get(i)));
            this->_init(i, static_cast<type&&>(r._get(i)));
        });
    }
    constexpr var_mctor& operator=(var_mctor&&) = default;
};

template <typename D, typename...>
struct var_masgn : D {
    using D::D;
    constexpr var_masgn() noexcept                   = default;
    constexpr var_masgn(const var_masgn&)            = default;
    constexpr var_masgn& operator=(const var_masgn&) = default;
    constexpr var_masgn(var_masgn&&)                 = default;
    constexpr var_masgn& operator=(var_masgn&&)      = delete;
};

template <typename D, trivial_move_assign... Ts>
struct var_masgn<D, Ts...> : D {
    using D::D;
    constexpr var_masgn() noexcept                                                           = default;
    constexpr var_masgn(const var_masgn&)                                                    = default;
    constexpr var_masgn& operator=(const var_masgn&)                                         = default;
    constexpr var_masgn(var_masgn&&)                                                         = default;
    constexpr var_masgn& operator=(var_masgn&& r) noexcept((nothrow_move_assign<Ts> && ...)) = default;
};

template <typename D, move_assign... Ts> requires nontrivial_move_assigns<Ts...>
struct var_masgn<D, Ts...> : D {
    using D::D;
    constexpr var_masgn() noexcept                   = default;
    constexpr var_masgn(const var_masgn&)            = default;
    constexpr var_masgn& operator=(const var_masgn&) = default;
    constexpr var_masgn(var_masgn&&)                 = default;
    constexpr var_masgn& operator=(var_masgn&& r) noexcept((nothrow_move_assign<Ts> && ...)) {
        idx_dispatch<sizeof...(Ts)>(r.index(), [&](auto i) {
            using type = decltype(auto(r._get(i)));
            this->_emplace(i, static_cast<type&&>(r._get(i)));
        });
        return *this;
    }
};

template <typename... Ts>
using var_impl4 = var_cctor<var_base<Ts...>, Ts...>;

template <typename... Ts>
using var_impl3 = var_mctor<var_impl4<Ts...>, Ts...>;

template <typename... Ts>
using var_impl2 = var_casgn<var_impl3<Ts...>, Ts...>;

template <typename... Ts>
using var_impl = var_masgn<var_impl2<Ts...>, Ts...>;

} // namespace dtls

class bad_var_access : public exception {
public:
    bad_var_access() = default;
    const char* what() const noexcept override {
        return "Bad variant access";
    }
};

template <typename... Ts>
struct var : dtls::var_impl<Ts...> {
    using var_base = dtls::var_base<Ts...>;
    using var_impl = dtls::var_impl<Ts...>;

    template <typename T>
    struct resolve_overload {
        type_t<T> operator()(T);
    };

    static constexpr auto size() noexcept {
        return size_c<sizeof...(Ts)>;
    }

    constexpr var() noexcept = delete;

    template <typename T, typename O = decltype(overloaded{resolve_overload<Ts>()...})>
        requires (!base_of<var_base, remove_const_ref<T>>)
    constexpr var(T&& value) noexcept(nothrow_ctor<typename decltype(O{}(value))::type, T&&>):
        var_impl(size_c<idx_of_type_pack<typename decltype(O{}(value))::type, var_base>>, fwd(value)) {}

    template <typename T>
    constexpr explicit var(type_t<T>, auto&&... args) noexcept(nothrow_ctor<T, decltype(args)...>):
        var_impl(size_c<idx_of_type_pack<T, var_base>>, fwd(args)...) {}

    template <typename T, typename O = decltype(overloaded{resolve_overload<Ts>()...})>
        requires (!base_of<var_base, remove_ref<T>>)
    constexpr var& operator=(T&& value) noexcept(nothrow_ctor<typename decltype(O{}(value))::type, T&&>) {
        var_base::_emplace(size_c<idx_of_type_pack<typename decltype(O{}(value))::type, var_base>>, fwd(value));
        return *this;
    }

    template <typename T>
    constexpr void emplace(auto&&... args) noexcept(nothrow_ctor<T, decltype(args)...>) {
        var_base::_emplace(size_c<idx_of_type_pack<T, var_base>>, fwd(args)...);
    }

    template <typename T>
    constexpr void emplace(type_t<T>, auto&&... args) noexcept(nothrow_ctor<T, decltype(args)...>) {
        emplace<T>(fwd(args)...);
    }

    constexpr bool valueless_by_exception() const noexcept {
        return false;
    }

    template <typename T>
    constexpr decltype(auto) unsafe_get(type_t<T> = {}) {
        return var_base::_get(size_c<idx_of_type_pack<T, var_base>>);
    }

    template <typename T>
    constexpr decltype(auto) unsafe_get(type_t<T> = {}) const {
        return var_base::_get(size_c<idx_of_type_pack<T, var_base>>);
    }

    template <typename T>
    constexpr decltype(auto) get(type_t<T> = {}) {
        constexpr auto idx = size_c<idx_of_type_pack<T, var_base>>;
        if (idx != var_base::index())
            throw bad_var_access();
        return var_base::_get(idx);
    }

    template <typename T>
    constexpr decltype(auto) get(type_t<T> = {}) const {
        constexpr auto idx = size_c<idx_of_type_pack<T, var_base>>;
        if (idx != var_base::index())
            throw bad_var_access();
        return var_base::_get(idx);
    }

    template <typename T>
    constexpr decltype(auto) operator[](type_t<T>) {
        return get<T>();
    }

    template <typename T>
    constexpr decltype(auto) operator[](type_t<T>) const {
        return get<T>();
    }

#if 0
    constexpr decltype(auto) visit(auto&&... functions) const& {
        return idx_dispatch<sizeof...(Ts)>(var_base::index(), [this, o = overloaded{fwd(functions)...}](auto i) {
            return o(var_base::_get(i));
        });
    }

    constexpr decltype(auto) visit(auto&&... functions) & {
        return idx_dispatch<sizeof...(Ts)>(var_base::index(), [this, o = overloaded{fwd(functions)...}](auto i) {
            return o(var_base::_get(i));
        });
    }

    constexpr decltype(auto) visit(auto&&... functions) && {
        return idx_dispatch<sizeof...(Ts)>(var_base::index(), [this, o = overloaded{fwd(functions)...}](auto i) {
            return o(mov(var_base::_get(i)));
        });
    }

    template <typename... Rs>
    static constexpr auto _determine_return_type(unique_type_list<Rs...> = {}) {
        if constexpr (sizeof...(Rs) == 1)
            return tag<Rs...>;
        else
            return tag<var<Rs...>>;
    }

    template <typename... RetTs>
    constexpr decltype(auto) _variable_return_visit(unique_type_list<RetTs...> tl, auto&&... functions) const& {
        using return_type = decltype(+_determine_return_type(tl));
        return typed_idx_dispatch<return_type, sizeof...(Ts)>(var_base::index(),
                                                              [this, o = overloaded{fwd(functions)...}](auto i) {
                                                                  return o(var_base::_get(i));
                                                              });
    }

    constexpr decltype(auto) visit_var(auto&&... functions) const& {
        using return_types = decltype(unique_type_list<>{}.merge_results(
            overloaded{fwd(functions)...}, type_list<Ts...>{}, type_list<void>{}));
        return _variable_return_visit(return_types{}, fwd(functions)...);
    }

    /* TODO: optional return */
    constexpr void visit_some(auto&&... functions) const& {
        idx_dispatch<sizeof...(Ts)>(var_base::index(), [this, o = overloaded{fwd(functions)...}](auto i) {
            if constexpr (requires { o(var_base::_get(i)); })
                o(var_base::_get(i));
        });
    }

    /* TODO: optional return */
    constexpr void visit_some(auto&&... functions) & {
        idx_dispatch<sizeof...(Ts)>(var_base::index(), [this, o = overloaded{fwd(functions)...}](auto i) {
            if constexpr (requires { o(var_base::_get(i)); })
                o(var_base::_get(i));
        });
    }

    /* TODO: optional return */
    constexpr void visit_some(auto&&... functions) && {
        idx_dispatch<sizeof...(Ts)>(var_base::index(), [this, o = overloaded{fwd(functions)...}](auto i) {
            if constexpr (requires { o(mov(var_base::_get(i))); })
                o(mov(var_base::_get(i)));
        });
    }
#endif
};

/* Visit */

namespace dtls {
constexpr decltype(auto) _visit(auto&& function, auto&& var) {
    return idx_dispatch<decltype(auto(var))::size()>(var.index(), [&](auto i) {
        return fwd(function)(fwd(var)._get(i));
    });
}

constexpr decltype(auto) _visit(auto&& function, auto&& var1, auto&& var2, auto&&... vars) {
    return idx_dispatch<decltype(auto(var1))::size()>(var1.index(), [&](auto i1) {
        return _visit(
            [&](auto&&... a) {
                return fwd(function)(fwd(var1)._get(i1), fwd(a)...);
            },
            fwd(var2),
            fwd(vars)...);
    });
}

template <typename F, typename... Ts>
struct _visit_caller {
    constexpr _visit_caller(type_list_t<F, Ts...>) {}
    constexpr decltype(auto) operator()(Ts... var, F function) const {
        return _visit(fwd(function), fwd(var)...);
    }
};
} // namespace dtls

constexpr decltype(auto) visit(auto&&... args) {
    return dtls::_visit_caller{type_list<decltype(args)...>.rotate_r()}(fwd(args)...);
}
} // namespace core

#undef fwd
