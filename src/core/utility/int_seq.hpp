#pragma once

#include "../basic_types.hpp"
#include "../int_const.hpp"

#define fwd(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace core {

namespace details {
    template <auto I, typename F>
    struct int_seq_reduce_helper {
        static inline constexpr auto idx = I;
        constexpr int_seq_reduce_helper(F func): handler(fwd(func)) {}
        F handler;
    };

    template <auto I, typename F>
    constexpr auto operator+(auto&& initial, int_seq_reduce_helper<I, F>&& h) {
        return h.handler(fwd(initial), int_c<h.idx>);
    }

    template <auto... Idxs>
    constexpr auto int_seq_reduce(auto&& initial, auto&& handler) {
        return (fwd(initial) + ... + int_seq_reduce_helper<Idxs, decltype(handler)>{fwd(handler)});
    }
}

template <typename T, T... nums>
struct int_seq {
    constexpr void foreach(auto&& handler) const {
        (handler(int_c<nums>), ...);
    }
    constexpr auto reduce(auto&& init, auto&& handler) const {
        return details::int_seq_reduce<nums...>(fwd(init), fwd(handler));
    }
};

template <size_t... Idxs>
using idx_seq = int_seq<size_t, Idxs...>;

#if __has_builtin(__make_integer_seq)
template <typename T, size_t size>
using make_int_seq = __make_integer_seq<int_seq, T, size>;

template <size_t size>
using make_idx_seq = __make_integer_seq<int_seq, size_t, size>;
#else
template <typename T, size_t size>
using make_int_seq = int_seq<T, __integer_pack(size)...>;

template <size_t size>
using make_idx_seq = int_seq<size_t, __integer_pack(size)...>;
#endif
} // namespace core

#undef fwd
