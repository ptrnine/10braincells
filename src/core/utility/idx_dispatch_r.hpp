#pragma once

#include "../int_const.hpp"

namespace core::dtls {
template <typename R, size_t S>
struct _idx_dispatch_r {
    template <size_t B>
    static constexpr R call(size_t i, auto&& func) {
        if (i < S / 2)
            return _idx_dispatch_r<R, S / 2>::template call<B>(i, static_cast<decltype(func)>(func));
        else
            return _idx_dispatch_r<R, S - S / 2>::template call<B + S / 2>(i - S / 2,
                                                                           static_cast<decltype(func)>(func));
    }
};

template <typename R>
struct _idx_dispatch_r<R, 0> {};

template <typename R>
struct _idx_dispatch_r<R, 1> {
    template <size_t B>
    static constexpr R call(size_t, auto&& func) {
        return static_cast<decltype(func)>(func)(int_const<B>{});
    }
};

#define default_unreachable() \
    default: __builtin_unreachable()
#define case_idx(I) \
    case I: return static_cast<decltype(func)>(func)(int_const<B + I>{});
#define def_idx_dispatch_r(N, ...)                       \
    template <typename R>                                \
    struct _idx_dispatch_r<R, N> {                       \
        template <size_t B>                              \
        static constexpr R call(size_t i, auto&& func) { \
            switch (i) {                                 \
                default_unreachable();                   \
                case_idx(0) __VA_ARGS__                  \
            }                                            \
        }                                                \
    }

    def_idx_dispatch_r(2, case_idx(1));
    def_idx_dispatch_r(3, case_idx(1) case_idx(2));
    def_idx_dispatch_r(4, case_idx(1) case_idx(2) case_idx(3));
    def_idx_dispatch_r(5, case_idx(1) case_idx(2) case_idx(3) case_idx(4));
    def_idx_dispatch_r(6, case_idx(1) case_idx(2) case_idx(3) case_idx(4) case_idx(5));
    def_idx_dispatch_r(7, case_idx(1) case_idx(2) case_idx(3) case_idx(4) case_idx(5) case_idx(6));
    def_idx_dispatch_r(8, case_idx(1) case_idx(2) case_idx(3) case_idx(4) case_idx(5) case_idx(6) case_idx(7));
    def_idx_dispatch_r(9,
                      case_idx(1) case_idx(2) case_idx(3) case_idx(4) case_idx(5) case_idx(6) case_idx(7)
                      case_idx(8));
    def_idx_dispatch_r(10,
                      case_idx(1) case_idx(2) case_idx(3) case_idx(4) case_idx(5) case_idx(6) case_idx(7)
                      case_idx(8) case_idx(9));
    def_idx_dispatch_r(11,
                      case_idx(1) case_idx(2) case_idx(3) case_idx(4) case_idx(5) case_idx(6) case_idx(7)
                      case_idx(8) case_idx(9) case_idx(10));
    def_idx_dispatch_r(12,
                      case_idx(1) case_idx(2) case_idx(3) case_idx(4) case_idx(5) case_idx(6) case_idx(7)
                      case_idx(8) case_idx(9) case_idx(10) case_idx(11));
    def_idx_dispatch_r(13,
                      case_idx(1) case_idx(2) case_idx(3) case_idx(4) case_idx(5) case_idx(6) case_idx(7)
                      case_idx(8) case_idx(9) case_idx(10) case_idx(11) case_idx(12));
    def_idx_dispatch_r(14,
                      case_idx(1) case_idx(2) case_idx(3) case_idx(4) case_idx(5) case_idx(6) case_idx(7)
                      case_idx(8) case_idx(9) case_idx(10) case_idx(11) case_idx(12) case_idx(13));
    def_idx_dispatch_r(15,
                      case_idx(1) case_idx(2) case_idx(3) case_idx(4) case_idx(5) case_idx(6) case_idx(7)
                      case_idx(8) case_idx(9) case_idx(10) case_idx(11) case_idx(12) case_idx(13)
                      case_idx(14));
    def_idx_dispatch_r(16,
                      case_idx(1) case_idx(2) case_idx(3) case_idx(4) case_idx(5) case_idx(6) case_idx(7)
                      case_idx(8) case_idx(9) case_idx(10) case_idx(11) case_idx(12) case_idx(13) case_idx(14)
                      case_idx(15));

#undef default_unreachable
#undef case_idx
#undef def_idx_dispatch_r

    } // namespace core::dtls

namespace core {
template <typename ReturnT, size_t S>
constexpr ReturnT idx_dispatch_r(size_t i, auto&& function) {
    return dtls::_idx_dispatch_r<ReturnT, S>::template call<0>(i, static_cast<decltype(function)>(function));
}
} // namespace core
