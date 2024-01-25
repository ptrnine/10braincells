#include <catch2/catch.hpp>

#include <core/nostd/concepts/integral.hpp>
#include <core/nostd/tuple.hpp>
#include <core/nostd/traits/is_same.hpp>
#include <core/nostd/utility/overloaded.hpp>

using core::integral;
using core::is_same;
using core::overloaded;
using core::tuple;
using core::u64;
using core::u8;

using namespace core::int_const_literals;

TEST_CASE("tuple") {
    SECTION("structured bindings") {
        tuple t{2, u64(2), 3};
        auto [a, b, c] = t;

        static_assert(is_same<decltype(a), int> && is_same<decltype(b), u64>);
        a = 100;
        b = 100;

        REQUIRE(a == 100);
        REQUIRE(b == 100);
        REQUIRE(t[0_c] == 2);
        REQUIRE(t[1_c] == 2);
        REQUIRE(t[2_c] == 3);

        auto&& [x, y, z] = t;
        static_assert(is_same<decltype(x), int> && is_same<decltype(y), u64>);
        x = 100;
        y = 200;

        REQUIRE(t[0_c] == 100);
        REQUIRE(t[1_c] == 200);

        auto&& [a1, b1, c1] = tuple{u8(255), u8(127), u8(0)};
        static_assert(is_same<decltype(a1), u8> && is_same<decltype(b1), u8>);

        REQUIRE(a1 == 255);
        REQUIRE(b1 == 127);
        REQUIRE(c1 == 0);

        a1 = 101;
        REQUIRE(a1 == 101);
    }

    SECTION("empty tuple") {
        auto t = tuple{};
        static_assert(t.size() == 0);
        static_assert(t.empty());

        bool never_true = false;
        t.foreach([&]() {
            never_true = true;
        });
        REQUIRE(!never_true);

        t.map([&] {
            never_true = true;
        });
        REQUIRE(!never_true);

        t.reduce(0, [&]{
            never_true = true;
        });
        REQUIRE(!never_true);
    }

    SECTION("foreach") {
        std::string res;
        tuple{"hello", 231}.foreach (overloaded{
            [&](integral auto v) { res += std::to_string(v); },
            [&](const char* v) { res += v; },
        });
        REQUIRE(res == "hello231");
    }

    SECTION("map") {
        auto t = tuple{std::string("hello"), 323};
        auto t2 = t.map(overloaded{
            [](const integral auto& v) -> decltype(auto) { return v; },
            [](const std::string& v) { return std::string_view(v); },
        });

        static_assert(is_same<decltype(t2), tuple<std::string_view, const int&>>);

        REQUIRE(t2[0_c] == "hello");
        REQUIRE(t2[1_c] ==  323);

        t[0_c].replace(0, 5, "qweqw");
        t[1_c] = 228;

        REQUIRE(t2[0_c] == "qweqw");
        REQUIRE(t2[1_c] ==  228);
    }
}
