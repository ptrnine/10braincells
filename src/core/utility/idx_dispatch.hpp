#pragma once

#include "../int_const.hpp"

namespace core::dtls {
template <size_t S>
struct _idx_dispatch {
    template <size_t B>
    static constexpr decltype(auto) call(size_t i, auto&& func) {
        if (i < S / 2)
            return _idx_dispatch<S / 2>::template call<B>(i, static_cast<decltype(func)>(func));
        else
            return _idx_dispatch<S - S / 2>::template call<B + S / 2>(i - S / 2, static_cast<decltype(func)>(func));
    }
};

template <>
struct _idx_dispatch<0> {};

template <>
struct _idx_dispatch<1> {
    template <size_t B>
    static constexpr decltype(auto) call(size_t, auto&& func) {
        return static_cast<decltype(func)>(func)(int_const<B>{});
    }
};

#define default_unreachable() \
    default: __builtin_unreachable()

#define case_idx(I) \
    case I: return static_cast<decltype(func)>(func)(int_const<B + I>{});

#define def_idx_dispatch(N, ...)                                      \
    template <>                                                       \
    struct _idx_dispatch<N> {                                         \
        template <size_t B>                                           \
        static constexpr decltype(auto) call(size_t i, auto&& func) { \
            switch (i) {                                              \
                default_unreachable();                                \
                case_idx(0) __VA_ARGS__                               \
            }                                                         \
        }                                                             \
    }

def_idx_dispatch(2, case_idx(1));
def_idx_dispatch(3, case_idx(1) case_idx(2));
def_idx_dispatch(4, case_idx(1) case_idx(2) case_idx(3));
def_idx_dispatch(5, case_idx(1) case_idx(2) case_idx(3) case_idx(4));
def_idx_dispatch(6, case_idx(1) case_idx(2) case_idx(3) case_idx(4) case_idx(5));
def_idx_dispatch(7, case_idx(1) case_idx(2) case_idx(3) case_idx(4) case_idx(5) case_idx(6));
def_idx_dispatch(8, case_idx(1) case_idx(2) case_idx(3) case_idx(4) case_idx(5) case_idx(6) case_idx(7));
def_idx_dispatch(9, case_idx(1) case_idx(2) case_idx(3) case_idx(4) case_idx(5) case_idx(6) case_idx(7) case_idx(8));
def_idx_dispatch(10,
                 case_idx(1) case_idx(2) case_idx(3) case_idx(4) case_idx(5) case_idx(6) case_idx(7) case_idx(8)
                 case_idx(9));
def_idx_dispatch(11,
                 case_idx(1) case_idx(2) case_idx(3) case_idx(4) case_idx(5) case_idx(6) case_idx(7) case_idx(8)
                 case_idx(9) case_idx(10));
def_idx_dispatch(12,
                 case_idx(1) case_idx(2) case_idx(3) case_idx(4) case_idx(5) case_idx(6) case_idx(7) case_idx(8)
                 case_idx(9) case_idx(10) case_idx(11));
def_idx_dispatch(13,
                 case_idx(1) case_idx(2) case_idx(3) case_idx(4) case_idx(5) case_idx(6) case_idx(7) case_idx(8)
                 case_idx(9) case_idx(10) case_idx(11) case_idx(12));
def_idx_dispatch(14,
                 case_idx(1) case_idx(2) case_idx(3) case_idx(4) case_idx(5) case_idx(6) case_idx(7) case_idx(8)
                 case_idx(9) case_idx(10) case_idx(11) case_idx(12) case_idx(13));
def_idx_dispatch(15,
                 case_idx(1) case_idx(2) case_idx(3) case_idx(4) case_idx(5) case_idx(6) case_idx(7) case_idx(8)
                 case_idx(9) case_idx(10) case_idx(11) case_idx(12) case_idx(13) case_idx(14));
def_idx_dispatch(16,
                 case_idx(1) case_idx(2) case_idx(3) case_idx(4) case_idx(5) case_idx(6) case_idx(7) case_idx(8)
                 case_idx(9) case_idx(10) case_idx(11) case_idx(12) case_idx(13) case_idx(14) case_idx(15));

#undef default_unreachable
#undef case_idx
#undef def_idx_dispatch

} // namespace core::dtls

namespace core {
template <size_t S>
constexpr decltype(auto) idx_dispatch(size_t i, auto&& function) {
    return dtls::_idx_dispatch<S>::template call<0>(i, static_cast<decltype(function)>(function));
}
} // namespace core
