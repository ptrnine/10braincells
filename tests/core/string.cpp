#include <catch2/catch_test_macros.hpp>

#include <core/string/path.hpp>

using namespace core;

TEST_CASE("path") {
    CHECK(is_relative_to("/home/test", "/home/test/lol/mda"));
    CHECK(is_relative_to("/home/test", "/home/test/lol/mda/"));
    CHECK(is_relative_to("/home/test/", "/home/test/lol/mda/"));
    CHECK(is_relative_to("/", "/home/test/lol/mda/"));
    CHECK(is_relative_to("/home/test", "/home/test/"));
    CHECK(is_relative_to("/home/test", "/home/test"));
    CHECK_FALSE(is_relative_to("/home/test", "/home"));
}
