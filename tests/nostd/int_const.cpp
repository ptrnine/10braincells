#include <catch2/catch.hpp>

#include "core/nostd/int_const.hpp"
using namespace core::int_const_literals;
using core::int_const;
using core::int_c;
using core::false_c;
using core::true_c;

#include "core/nostd/traits/is_same.hpp"
using core::is_same;

TEST_CASE("int_const") {
    SECTION("literals") {
        static_assert(0xdeadbeef_c == 0xdeadbeef);
        static_assert(0x0_c == 0);
        static_assert(0b1101010101_c == 0b1101010101);
        static_assert(0xff'ff'ff_c == 0xff'ff'ff);
        static_assert(1'000'000'000'000_c == 1000000000000);
        static_assert(true_c);
        static_assert(!false_c);

        static_assert(is_same<decltype(0xff_c)::type, int>);
        static_assert(is_same<decltype(0x7fffffff_c)::type, int>);
        static_assert(is_same<decltype(0x8fffffff_c)::type, unsigned int>);
        static_assert(is_same<decltype(0xffffffff_c)::type, unsigned int>);
        static_assert(is_same<decltype(0x1ffffffff_c)::type, long int>);
        static_assert(is_same<decltype(0x7fffffffffffffff_c)::type, long>);
        static_assert(is_same<decltype(0x8fffffffffffffff_c)::type, unsigned long>);

        static_assert(is_same<decltype(0xff_uc)::type, unsigned int>);
        static_assert(is_same<decltype(0xffffffff_uc)::type, unsigned int>);
        static_assert(is_same<decltype(0xfffffffff_uc)::type, unsigned long>);
        static_assert(is_same<decltype(0xff_ulc)::type, unsigned long>);
        static_assert(is_same<decltype(0xff_ullc)::type, unsigned long long>);
        static_assert(is_same<decltype(0x7fffffff'ffffffff_lc)::type, long>);
        static_assert(is_same<decltype(0x8fffffff'ffffffff_lc)::type, unsigned long>);
        static_assert(is_same<decltype(0x7fffffff'ffffffff_llc)::type, long long>);
        static_assert(is_same<decltype(0x8fffffff'ffffffff_llc)::type, unsigned long long>);

        static_assert(is_same<decltype(0x7fffffff'ffffffff_zc)::type, core::ssize_t>);
        static_assert(is_same<decltype(0x8fffffff'ffffffff_zc)::type, core::size_t>);
        static_assert(is_same<decltype(0xff_zuc)::type, core::size_t>);
        static_assert(is_same<decltype(0xffffffff'ffffffff_zc)::type, core::size_t>);
    }

    SECTION("operators") {
        static_assert(0xf_c == 0xf_c);
        static_assert(2_c > -1_c);
        static_assert(2_c >= 1_c);
        static_assert(-1_c < 2_c);
        static_assert(1_c <= 1_c);
        static_assert(int_c<char(2)> == 2_c);
        static_assert(-1_c == -1);
        static_assert(+-1_c == -1);
        static_assert(2_c * 2_c == 4_c);
        static_assert(0_c - 20_c == -20_c);
        static_assert(4_c / 2_c == 2_c);
        static_assert((4_c && 2_c) == true_c);
        static_assert((0_c || 0_c) == false_c);
        static_assert(5_c % 2_c == 1_c);
        static_assert((0xff_c ^ 0x0f_c) == 0xf0_c);
        static_assert((0x0f_c | 0xf0_c) == 0xff_c);
        static_assert((0xf0_c & 0xff_c) == 0xf0_c);
        static_assert((0xf_c << 4_c) == 0xf0_c);
        static_assert((0xff_c >> 4_c) == 0xf_c);
        static_assert(~0_c == 0xffffffff_c);
    }
}
