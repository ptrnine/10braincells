#include <catch2/catch_test_macros.hpp>

#include <core/hash.hpp>
#include <core/opt.hpp>
#include <core/var.hpp>

#include <cmath>

#include <string>
#include <string_view>
#include <vector>

using namespace core;

TEST_CASE("hash: primitives") {
    REQUIRE(hash(u64{42}) != 0);
    REQUIRE(hash(u64{42}) == hash(u64{42}));
    REQUIRE(hash(u64{42}) != hash(u64{43}));
    REQUIRE(hash(0u) != hash(1u));
}

TEST_CASE("hash: strings") {
    REQUIRE(hash(std::string{"abc"}) == xxh64("abc", 3));
    REQUIRE(hash(std::string_view{"abc"}) == hash(std::string{"abc"}));
    // xxHash mixes in the length, so "ab" and "ab\0" must differ
    REQUIRE(hash(std::string{"ab"}) != hash(std::string("ab\0", 3)));
    REQUIRE(hash(std::string{}) == hash(std::string_view{}));
}

TEST_CASE("hash: c-strings") {
    const char* a = "abc";
    const char* b = "abc"; // different address, same content
    REQUIRE(hash(a) == hash(b));
    REQUIRE(hash(a) != hash(const_cast<const char*>("abcd")));
    REQUIRE(hash(a) == fnv1a64_nullterm("abc"));
    // must not be hashed by address: same content, distinct objects
    std::string s1{"abc"}, s2{"abc"};
    REQUIRE(hash(s1.data()) == hash(s2.data()));
}

TEST_CASE("hash: floating point") {
    REQUIRE(hash(1.5f) == hash(1.5f));
    REQUIRE(hash(1.5f) != hash(1.5000001f));
    REQUIRE(hash(1.0) == hash(1.0));
    REQUIRE(hash(1.0f) != hash(1.0)); // different type/size
    // -0.0 == +0.0 and must hash equal
    REQUIRE(hash(-0.0f) == hash(0.0f));
    // NaN != anything, but all NaNs hash equal
    auto nan1 = static_cast<float>(std::nan(""));
    auto nan2 = -nan1;
    REQUIRE((std::isnan(nan1) && std::isnan(nan2)));
    REQUIRE(hash(nan1) == hash(nan2));
    REQUIRE(hash(nan1) != hash(0.0f));
    REQUIRE(hash(nan1) != hash(1.0f));
    REQUIRE(hash(std::nan("")) == hash(-std::nan("")));
    REQUIRE(hash(1.0L) == hash(1.0L));
}

TEST_CASE("hash: byteview") {
    int x = 42;
    core::byteview v(x);
    REQUIRE(hash(v) == xxh64("\x2a\x00\x00\x00", 4)); // little-endian 42
    char buf[8]   = {1, 2, 3, 4, 5, 6, 7, 8};
    char buf2[8]  = {1, 2, 3, 4, 5, 6, 7, 8};
    char buf3[8]  = {1, 2, 3, 4, 5, 6, 7, 9};
    core::byteview v1(buf), v2(buf2);
    REQUIRE(hash(v1) == hash(v2)); // same content, different buffers
    REQUIRE(hash(v1) == xxh64(buf, 8));
    core::byteview v3(buf3);
    REQUIRE(hash(v1) != hash(v3));
}

TEST_CASE("hash: trivially copyable fallback") {
    struct point { u32 x; u32 y; };
    point p{1, 2};
    REQUIRE(hash(p) == hash(point{1, 2}));
    REQUIRE(hash(p) != hash(point{1, 3}));
    REQUIRE(hash(p) == fnv1a64(&p, sizeof(p)));
    // empty struct
    struct empty_t {};
    REQUIRE(hash(empty_t{}) == hash(empty_t{}));
    // nesting through containers
    REQUIRE(hash(std::vector<point>{point{1, 2}, point{3, 4}}) == hash(std::vector<point>{point{1, 2}, point{3, 4}}));
    REQUIRE(hash(var<point, float>{point{1, 2}}) != hash(var<point, float>{1.5f}));
}

TEST_CASE("hash: vector") {
    std::vector<int> v{1, 2, 3};
    REQUIRE(hash(v) == hash(std::vector<int>{1, 2, 3}));
    REQUIRE(hash(v) != hash(std::vector<int>{1, 2, 3, 4})); // length affects hash
    REQUIRE(hash(v) != hash(std::vector<int>{1, 2, 4}));
    REQUIRE(hash(std::vector<int>{}) == hash(std::vector<int>{}));
    REQUIRE(hash(std::vector<std::string>{"x", "y"}) == hash(std::vector<std::string>{"x", "y"}));
}

TEST_CASE("hash: opt") {
    auto empty = hash(opt<int>{});
    auto some  = hash(opt<int>{7});
    REQUIRE(empty != some);
    REQUIRE(hash(opt<int>{7}) == some);
    REQUIRE(hash(opt<int>{8}) != some);
    // embedded in a variant: empty and filled must differ
    REQUIRE(hash(var<opt<int>, char>{opt<int>{}}) != hash(var<opt<int>, char>{opt<int>{7}}));
}

TEST_CASE("hash: var") {
    var<int, std::string> vi{3}, vi2{3}, vs{"3"};
    REQUIRE(hash(vi) == hash(vi2));
    REQUIRE(hash(vi) != hash(vs)); // different active alternative
    // alternative index participates in the hash: same logical value, different slot
    var<std::string, char> sv{"3"}, sc{'3'};
    REQUIRE(hash(sv) != hash(sc));

    var<opt<int>, std::string> vo{opt<int>{7}}, vo2{opt<int>{7}}, vo3{std::string{"x"}};
    REQUIRE(hash(vo) == hash(vo2));
    REQUIRE(hash(vo) != hash(vo3));
    REQUIRE(hash(var<opt<int>, std::string>{opt<int>{}}) != hash(vo));

    // nesting
    var<std::vector<int>, opt<array<int, 2>>> nv{std::vector<int>{1, 2}}, nv2{std::vector<int>{1, 2}},
                                              nv3{opt<array<int, 2>>{}};
    REQUIRE(hash(nv) == hash(nv2));
    REQUIRE(hash(nv) != hash(nv3));
}

TEST_CASE("hash: array") {
    array<int, 3> a{1, 2, 3};
    REQUIRE(hash(a) == hash(array<int, 3>{1, 2, 3}));
    REQUIRE(hash(a) != hash(array<int, 3>{1, 2, 4}));

    // multidimensional
    array<int, 2, 2> m;
    m[0][0] = 1; m[0][1] = 2; m[1][0] = 3; m[1][1] = 4;
    array<int, 2, 2> m2 = m;
    array<int, 2, 2> m3 = m;
    m3[1][1] = 5;
    REQUIRE(hash(m) == hash(m2));
    REQUIRE(hash(m) != hash(m3));

    array<std::string, 2> sa{"x", "y"};
    REQUIRE(hash(sa) == hash(array<std::string, 2>{"x", "y"}));
}
