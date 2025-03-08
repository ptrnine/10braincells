#include <catch2/catch_test_macros.hpp>
#include <core/meta/loophole.hpp>

enum class test_base_tag {};

template <core::size_t idx, typename Derived>
struct test_base : core::closed_set_type<test_base_tag, idx, Derived> {};

#define CstTest(Name) Name : test_base<core::lh::count_and_next<test_base_tag>(), Name>

struct CstTest(test1) {
    constexpr test1(int ia): a(ia) {}
    int a;
};

struct CstTest(test2) {
    int a;
};

TEST_CASE("closed_set_type") {
    SECTION("idx_to_type") {
        static_assert(test1::cst_idx == 0);
        static_assert(test2::cst_idx == 1);
        static_assert(core::lh::get_type_from_idx<test_base_tag, 0>() == core::type<test1>);
        static_assert(core::lh::get_type_from_idx<test_base_tag, 1>() == core::type<test2>);
        static_assert(core::lh::get_type_from_idx<test_base_tag, 2>() == core::type<core::lh::type_not_found>);
    }

    SECTION("count") {
        static_assert(core::lh::count<test_base_tag>() == 2);
    }
}

TEST_CASE("lh_counter") {
    using tag = void;
    static_assert(core::lh::count<tag>() == 0);

    static_assert(core::lh::count_and_next<tag>() == 0);
    static_assert(core::lh::count<tag>() == 1);

    static_assert(core::lh::count_and_next<tag>() == 1);
    static_assert(core::lh::count_and_next<tag>() == 2);
    static_assert(core::lh::count_and_next<tag>() == 3);
    static_assert(core::lh::count<tag>() == 4);

    using new_tag = int;
    static_assert(core::lh::count<new_tag>() == 0);
    static_assert(core::lh::count_and_next<new_tag>() == 0);
    static_assert(core::lh::count_and_next<new_tag>() == 1);
    static_assert(core::lh::count_and_next<new_tag>() == 2);
    static_assert(core::lh::count<new_tag>() == 3);
    static_assert(core::lh::count<tag>() == 4);
}
