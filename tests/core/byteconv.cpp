#include <catch2/catch_test_macros.hpp>

#include <map>
#include <string>

#include <core/byteconv.hpp>

using core::type;
using core::to_bytes;
using core::from_bytes;

TEST_CASE("byteconv") {
    SECTION("string") {
        std::string v = "hello!";

        auto bs = to_bytes(v);
        static_assert(type<decltype(bs)> == type<std::vector<char>>);
        REQUIRE(bs == std::vector<char>{0x6, 0, 0, 0, 'h', 'e', 'l', 'l', 'o', '!'});

        auto r = from_bytes<std::string>(bs);
        REQUIRE(r.size() == 6);
        REQUIRE(r == "hello!");
    }

    SECTION("map") {
        std::map<std::string, int> v = {{"key", 1}, {"key2", 228}};

        auto bs = to_bytes(v);
        static_assert(type<decltype(bs)> == type<std::vector<char>>);
        REQUIRE(bs == std::vector<char>{0x2, 0, 0, 0, 0x3, 0, 0, 0, 'k', 'e', 'y', 0x1, 0, 0, 0, 0x4, 0, 0, 0, 'k', 'e', 'y', '2', char(228), 0, 0, 0});

        //auto r = from_bytes<std::map<std::string, int>>(bs);
    }
}
