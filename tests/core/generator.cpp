#include <catch2/catch_test_macros.hpp>

#include <vector>

#include <core/generator.hpp>

using namespace core;

TEST_CASE("generator") {
    auto test = [] -> generator<int> {
        for (int i = 0; i < 10; ++i)
            co_yield i;
    };

    int n = 0;
    for (auto i : test()) {
        CHECK(n++ == i);
    }
    CHECK(n == 10);
}
