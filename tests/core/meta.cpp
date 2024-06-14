#include <catch2/catch_test_macros.hpp>

#include "core/meta/nttp.hpp"
using core::nttp;

TEST_CASE("nttp") {
    auto a = nttp<2323>;
    static_assert(+a == 2323);
    static_assert(a == 2323);

    struct test {
        auto operator<=>(const test&) const = default;
        int a, b, c, d;
    };
    auto b = nttp<test{1, 2, 3, 4}>;

    static_assert(b == test{1, 2, 3, 4});
}

#include "core/meta/type.hpp"
#include "core/traits/is_same.hpp"
using core::type;
using core::type_of;
using core::is_same;
using core::decl_type;

TEST_CASE("type") {
    auto t1 = type<int>;

    int value = 2323121;
    auto t2 = type_of(value);

    static_assert(t1 == t2);
    static_assert(is_same<decl_type<t1()>, decl_type<t2()>>);
    static_assert(is_same<decl_type<t1()>, int>);

    /* Array type */
    auto t3 = type<int[]>;
    static_assert(t3 == type<int[]>);
}

#include "core/meta/type_list.hpp"
using core::type_list;
using core::type;
using core::indexed_type;
using namespace core::int_const_literals;

TEST_CASE("type_list") {
    SECTION("compare") {
        static_assert(type_list<int, float, int> == type_list<int, float, int>);
        static_assert(type_list<int, float> != type_list<float, int>);
        static_assert(type_list<> == type_list<>);
    }

    SECTION("dedup") {
        static_assert(type_list<int, int, float, double, float>.dedup() == type_list<int, float, double>);
    }

    SECTION("operator[]") {
        static_assert(type_list<int, float, void>[1_c] == type<float>);
    }

    SECTION("pop_front") {
        static_assert(type_list<int, char, float>.pop_front() == type_list<char, float>);
    }

    SECTION("append") {
        static_assert(type_list<>.append(type<int>) == type_list<int>);
        static_assert(type_list<float>.append(type_list<int, int>) == type_list<float, int, int>);
    }

    SECTION("prepend") {
        static_assert(type_list<float>.prepend(type_list<int>) == type_list<int, float>);
        static_assert(type_list<int>.prepend(type<double>) == type_list<double, int>);
    }

    SECTION("rotate_l") {
        static_assert(type_list<int, char, float>.rotate_l() == type_list<char, float, int>);
    }

    SECTION("rotate_r") {
        static_assert(type_list<int, char, float>.rotate_r() == type_list<float, int, char>);
    }

    SECTION("size") {
        static_assert(type_list<>.size() == 0);
        static_assert(type_list<int>.size() == 1);
        static_assert(type_list<int, float>.size() == 2);
    }

    SECTION("operator+") {
        static_assert((type<int> + type<char>) == type_list<int, char>);
        static_assert((type_list<int, char> + type<void>) == type_list<int, char, void>);
        static_assert((type<void> + type_list<int, char>) == type_list<void, int, char>);
        static_assert((type_list<> + type_list<>) == type_list<>);
        static_assert((type_list<int> + type_list<void>) == type_list<int, void>);
    }

    SECTION("foreach") {
        std::string result;
        type_list<int, char, float>.foreach ([&](auto t) {
            if constexpr (t == type<int>)
                result += "int";
            else if constexpr (t == type<char>)
                result += "char";
            else if constexpr (t == type<float>)
                result += "float";
        });
        REQUIRE(result == "intcharfloat");
    }

    SECTION("map") {
        static_assert(type_list<int, char>.map([](auto t) {
            return type<std::vector<decl_type<t()>>>;
        }) == type_list<std::vector<int>, std::vector<char>>);
    }

    SECTION("reduce") {
        auto list = type_list<int, char>.reduce(type_list<>, [](auto v, auto t) {
            return v + (type<void> + t);
        });
        static_assert(list == type_list<void, int, void, char>);
    }

    SECTION("dispatch") {
        size_t called = 0;
        type_list<int, void, char>.dispatch(1, [&](auto t) {
            ++called;
            REQUIRE(t == type<void>);
        });
        REQUIRE(called == 1);
    }

    SECTION("indexed") {
        static_assert(type_list<int, void, float>.indexed() ==
                      type_list<indexed_type<0, int>, indexed_type<1, void>, indexed_type<2, float>>);
    }
}
