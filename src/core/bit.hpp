#pragma once

#include <core/basic_types.hpp>
#include <core/int_const.hpp>
#include <core/limits.hpp>

namespace core // NOLINT
{
namespace details
{
#ifdef __GNUC__
    inline constexpr u32 next_pow2(u32 x) {
        return x == 1 ? 2
                      : (x > (limits<u32>::max() / 2 + 1)
                             ? limits<u32>::max()
                             : (1 << (32U - u32(__builtin_clz(x - 1)))));
    }
    inline constexpr u64 next_pow2(u64 x) {
        return x == 1 ? 2
                      : (x > (limits<u64>::max() / 2 + 1)
                             ? limits<u64>::max()
                             : (1 << (64U - u64(__builtin_clzll(x - 1)))));
    }
#else
    inline constexpr u32 next_pow2(u32 x) {
        if (x == 1)
            return 2;
        else {
            --x;
            x |= x >> 1;
            x |= x >> 2;
            x |= x >> 4;
            x |= x >> 8;
            x |= x >> 16;
            ++x;
            return x;
        }
    }
    inline constexpr u64 next_pow2(u64 x) {
        if (x == 1)
            return 2;
        else {
            --x;
            x |= x >> 1;
            x |= x >> 2;
            x |= x >> 4;
            x |= x >> 8;
            x |= x >> 16;
            x |= x >> 32;
            ++x;
            return x;
        }
    }

#endif
    inline constexpr u8 next_pow2(u8 x) {
        if (x == 1)
            return 2;
        else {
            --x;
            x |= x >> 1;
            x |= x >> 2;
            x |= x >> 4;
            ++x;
            return x;
        }
    }
    inline constexpr u16 next_pow2(u16 x) {
        if (x == 1)
            return 2;
        else {
            --x;
            x |= x >> 1;
            x |= x >> 2;
            x |= x >> 4;
            x |= x >> 8;
            ++x;
            return x;
        }
    }

} // namespace details

constexpr inline i8 ilog2(u64 value) {
    constexpr i8 table[64] = {
        63,  0, 58,  1, 59, 47, 53,  2,
        60, 39, 48, 27, 54, 33, 42,  3,
        61, 51, 37, 40, 49, 18, 28, 20,
        55, 30, 34, 11, 43, 14, 22,  4,
        62, 57, 46, 52, 38, 26, 32, 41,
        50, 36, 17, 19, 29, 10, 13, 21,
        56, 45, 25, 31, 35, 16,  9, 12,
        44, 24, 15,  8, 23,  7,  6,  5
    };

    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    value |= value >> 32;

    return table[((value - (value >> 1)) * 0x07EDD5E59A4E28C2) >> 58];
}

constexpr inline i8 ilog2(u32 value) {
    constexpr i8 table[32] = {
        0,  9,  1,  10, 13, 21, 2,  29,
        11, 14, 16, 18, 22, 25, 3,  30,
        8,  12, 20, 28, 15, 17, 24, 7,
        19, 27, 23, 6,  26, 5,  4,  31
    };

    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    return table[value * 0x07C4ACDD >> 27];
}

constexpr inline i8 ilog2(u16 value) {
    return ilog2(u32(value));
}

constexpr inline i8 ilog2(u8 value) {
    return ilog2(u32(value));
}

constexpr auto next_power_of_two(auto number) {
    return number == int_c<0> ? number : details::next_pow2(number);
}

constexpr auto is_pow_of_2(auto v) {
    return (v & (v - int_c<2>)) == int_c<0>;
}

namespace details {
    template <typename T>
    struct is_pow_of_2_assert {};

    template <auto V>
    struct is_pow_of_2_assert<int_const<V>> {
        static_assert(is_pow_of_2(V), "\"to\" must be power of 2");
    };
}

constexpr auto round_up(auto value, auto to) {
    details::is_pow_of_2_assert<decltype(to)>{};
    return ((value + (to - int_c<1>)) & ~(to - int_c<1>));
}
} // namespace core
