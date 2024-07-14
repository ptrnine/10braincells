#include <catch2/catch_test_macros.hpp>

#include <list>

#include <core/ranges/zip.hpp>

using core::tuple;
using core::type;
using core::type_of;
using core::zip;
using core::zip_view;

TEST_CASE("zip") {
    SECTION("simple for") {
        std::vector<int> a{1, 2, 3, 4, 5};
        std::list<int>   b{2, 3, 4, 5, 6, 7};

        std::vector<int> res;
        for (auto [x, y] : zip_view{a, b}) res.push_back(x + y);

        REQUIRE(res == std::vector{3, 5, 7, 9, 11});
    }

    SECTION("modify") {
        std::vector a{1, 2, 3, 4, 5};
        std::vector b{2, 1, 2, 2, 1};
        for (auto [x, y] : zip{a, b})
            x *= y;

        REQUIRE(a == std::vector{2, 2, 6, 8, 5});
    }

    SECTION("move accept") {
        std::vector v{1.f, 3.f};
        zip         z{std::vector{1, 2, 3}, v};
        static_assert(type_of(z.containers) == type<tuple<std::vector<int>, std::vector<float>&>>);
    }
}

#include <core/ranges/subst.hpp>

using core::views::subst;
using core::views::subst_entry;

TEST_CASE("subst") {
    auto sbst = []<typename C>(const C* str, const auto&... args) {
        decltype(std::basic_string<C>{str}) res;
        for (auto&& v : str | subst{args...})
            res += std::basic_string<C>{v};
        return res;
    };

    CHECK(sbst("test", subst_entry{"key", "value"}) == "test");
    CHECK(sbst("", subst_entry{"key", "value"}).empty());
    CHECK(sbst("k=${v} ${v} 123${v2}lol", subst_entry{"v", "KEY"}, subst_entry{"v2", "TEST"}) == "k=KEY KEY 123TESTlol");
    CHECK(sbst("${v} ${v} 123${v2}", subst_entry{"v", "KEY"}, subst_entry{"v2", "TEST"}) == "KEY KEY 123TEST");
    CHECK(sbst("a${}a", subst_entry{"", "lol"}) == "alola");
    CHECK(sbst("asd${", subst_entry{"", "lol"}) == "asdlol");
    CHECK(sbst("asd${", subst_entry{"asd", "lol"}) == "asd");
    CHECK(sbst("asd$34", subst_entry{"asd", "lol"}) == "asd$34");
}
