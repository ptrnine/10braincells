#pragma once

#include <core/limits.hpp>

namespace core {
template <typename T>
constexpr T rotl(T x, int s) noexcept {
    constexpr auto nd = limits<T>::digits;
    if constexpr ((nd & (nd - 1)) == 0) {
        constexpr unsigned und = nd;
        const unsigned r = s;
        return T(x << (r % und)) | T(x >> ((-r) % und));
    }
    const int r = s % nd;
    if (r == 0)
        return x;
    else if (r > 0)
        return T(x << r) | T(x >> ((nd - r) % nd));
    else
        return T(x >> -r) | T(x << ((nd + r) % nd));
}

template <typename T>
constexpr T rotr(T x, int s) noexcept {
    constexpr auto nd = limits<T>::digits;
    if constexpr ((nd & (nd - 1)) == 0) {
        constexpr unsigned und = nd;
        const unsigned r = s;
        return (x >> (r % und)) | (x << ((-r) % und));
    }
    const int r = s % nd;
    if (r == 0)
        return x;
    else if (r > 0)
        return (x >> r) | (x << ((nd - r) % nd));
    else
        return (x << -r) | (x >> ((nd + r) % nd));
}
} // namespace core
