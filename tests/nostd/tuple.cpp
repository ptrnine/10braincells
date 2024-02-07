#include <catch2/catch_test_macros.hpp>

#include <tuple>

#include <core/nostd/concepts/integral.hpp>
#include <core/nostd/tuple.hpp>
#include <core/nostd/traits/is_same.hpp>
#include <core/nostd/utility/overloaded.hpp>
#include <core/nostd/var.hpp>
#include <core/nostd/traits/is_same.hpp>
#include <core/nostd/utility/as_const.hpp>

using core::integral;
using core::is_same;
using core::overloaded;
using core::tuple;
using core::u64;
using core::u8;
using core::var;
using core::mov;
using core::fwd_as_tuple;
using core::declval;
using core::as_const;

using namespace core::int_const_literals;

struct no_mov_ctor_handler {
    no_mov_ctor_handler()                      = default;
    no_mov_ctor_handler(no_mov_ctor_handler&&) = delete;
    auto operator()(auto...) { return 1; }
};

TEST_CASE("tuple") {
    SECTION("structured bindings") {
        tuple t{2, u64(2), 3};
        auto [a, b, c] = t;

        static_assert(is_same<decltype(a), int> && is_same<decltype(b), u64>);
        a = 100;
        b = 100;

        std::tuple t2{2, u64(2), 3};
        auto [a2, b2, c2] = t2;
        static_assert(is_same<decltype(a2), int> && is_same<decltype(b2), u64>);

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

    SECTION("tuple_element") {
        using t1 = tuple<int, float&&>;
        using t2 = std::tuple<int, float&&>;

        static_assert(is_same<std::tuple_element_t<0, t1>, int>);
        static_assert(is_same<std::tuple_element_t<1, t1>, float&&>);
        static_assert(is_same<std::tuple_element_t<0, t2>, int>);
        static_assert(is_same<std::tuple_element_t<1, t2>, float&&>);
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

    SECTION("get") {
        tuple t1{2, std::string("hello")};
        std::tuple t2{2, std::string("hello")};

        static_assert(is_same<decltype(get<1>(t1)), std::string&>);
        static_assert(is_same<decltype(t1[1_c]), std::string&>);
        static_assert(is_same<decltype(get<1>(t2)), std::string&>);

        static_assert(is_same<decltype(get<1>(mov(t1))), std::string&&>);
        static_assert(is_same<decltype(mov(t1)[1_c]), std::string&>);
        static_assert(is_same<decltype(std::array<std::string, 1>{"qwe"}[0]), std::string&>);
        static_assert(is_same<decltype(get<1>(mov(t2))), std::string&&>);
    }

    SECTION("forward_as_tuple") {
        static_assert(is_same<decltype(fwd_as_tuple(2, 2.2, std::string())), tuple<int&&, double&&, std::string&&>>);
        static_assert(is_same<decltype(std::forward_as_tuple(2, 2.2, std::string())), std::tuple<int&&, double&&, std::string&&>>);
    }

    SECTION("forward") {
        int a = 1, b = 2;

        tuple<int, int>               v_tuple_v{a, b};
        tuple<int&, int&>             v_tuple_lv{a, b};
        tuple<const int&, const int&> v_tuple_clv{a, b};
        tuple<int&&, int&&>           v_tuple_rv{mov(a), mov(b)};

        auto& lv_tuple_v = v_tuple_v;
        auto& lv_tuple_lv = v_tuple_lv;
        auto& lv_tuple_clv = v_tuple_clv;
        auto& lv_tuple_rv = v_tuple_rv;

        const auto& clv_tuple_v = v_tuple_v;
        const auto& clv_tuple_lv = v_tuple_lv;
        const auto& clv_tuple_clv = v_tuple_clv;
        const auto& clv_tuple_rv = v_tuple_rv;

        /* Lvalue tuple */
        static_assert(is_same<decltype(v_tuple_v.forward()), tuple<int&, int&>>);
        static_assert(is_same<decltype(v_tuple_lv.forward()), tuple<int&, int&>>);
        static_assert(is_same<decltype(v_tuple_clv.forward()), tuple<const int&, const int&>>);
        static_assert(is_same<decltype(v_tuple_rv.forward()), tuple<int&, int&>>);

        static_assert(is_same<decltype(lv_tuple_v.forward()), tuple<int&, int&>>);
        static_assert(is_same<decltype(lv_tuple_lv.forward()), tuple<int&, int&>>);
        static_assert(is_same<decltype(lv_tuple_clv.forward()), tuple<const int&, const int&>>);
        static_assert(is_same<decltype(lv_tuple_rv.forward()), tuple<int&, int&>>);

        /* Const lvalue tuple */
        static_assert(is_same<decltype(clv_tuple_v.forward()), tuple<const int&, const int&>>);
        static_assert(is_same<decltype(clv_tuple_lv.forward()), tuple<const int&, const int&>>);
        static_assert(is_same<decltype(clv_tuple_clv.forward()), tuple<const int&, const int&>>);
        static_assert(is_same<decltype(clv_tuple_rv.forward()), tuple<const int&, const int&>>);

        /* Rvalue tuple */
        static_assert(is_same<decltype(mov(v_tuple_v).forward()), tuple<int&&, int&&>>);
        static_assert(is_same<decltype(mov(v_tuple_lv).forward()), tuple<int&, int&>>);
        static_assert(is_same<decltype(mov(v_tuple_clv).forward()), tuple<const int&, const int&>>);
        static_assert(is_same<decltype(mov(v_tuple_rv).forward()), tuple<int&&, int&&>>);
    }

    SECTION("move") {
        int a = 1, b = 2;

        tuple<int, int>               v_tuple_v{a, b};
        tuple<int&, int&>             v_tuple_lv{a, b};
        tuple<const int&, const int&> v_tuple_clv{a, b};
        tuple<int&&, int&&>           v_tuple_rv{mov(a), mov(b)};

        auto& lv_tuple_v = v_tuple_v;
        auto& lv_tuple_lv = v_tuple_lv;
        auto& lv_tuple_clv = v_tuple_clv;
        auto& lv_tuple_rv = v_tuple_rv;

        const auto& clv_tuple_v = v_tuple_v;
        const auto& clv_tuple_lv = v_tuple_lv;
        const auto& clv_tuple_clv = v_tuple_clv;
        const auto& clv_tuple_rv = v_tuple_rv;

        /* Lvalue tuple */
        static_assert(is_same<decltype(v_tuple_v.move()), tuple<int&&, int&&>>);
        static_assert(is_same<decltype(v_tuple_lv.move()), tuple<int&&, int&&>>);
        static_assert(is_same<decltype(v_tuple_clv.move()), tuple<const int&&, const int&&>>);
        static_assert(is_same<decltype(v_tuple_rv.move()), tuple<int&&, int&&>>);

        static_assert(is_same<decltype(lv_tuple_v.move()), tuple<int&&, int&&>>);
        static_assert(is_same<decltype(lv_tuple_lv.move()), tuple<int&&, int&&>>);
        static_assert(is_same<decltype(lv_tuple_clv.move()), tuple<const int&&, const int&&>>);
        static_assert(is_same<decltype(lv_tuple_rv.move()), tuple<int&&, int&&>>);

        /* Const lvalue tuple */
        static_assert(is_same<decltype(clv_tuple_v.move()), tuple<const int&&, const int&&>>);
        static_assert(is_same<decltype(clv_tuple_lv.move()), tuple<const int&&, const int&&>>);
        static_assert(is_same<decltype(clv_tuple_clv.move()), tuple<const int&&, const int&&>>);
        static_assert(is_same<decltype(clv_tuple_rv.move()), tuple<const int&&, const int&&>>);

        /* Rvalue tuple */
        static_assert(is_same<decltype(mov(v_tuple_v).move()), tuple<int&&, int&&>>);
        static_assert(is_same<decltype(mov(v_tuple_lv).move()), tuple<int&&, int&&>>);
        static_assert(is_same<decltype(mov(v_tuple_clv).move()), tuple<const int&&, const int&&>>);
        static_assert(is_same<decltype(mov(v_tuple_rv).move()), tuple<int&&, int&&>>);
    }

    SECTION("cat") {
        tuple       v{1};
        auto&       lv  = v;
        const auto& clv = v;

        /*=================== tuple<int> and tuple<int> ========================*/

        /* Lv and Lv */
        static_assert(is_same<decltype(v.cat(v)), tuple<int, int>>);
        static_assert(is_same<decltype(v.cat(v.forward())), tuple<int, int&>>);
        static_assert(is_same<decltype(v.cat(v.move())), tuple<int, int&&>>);
        static_assert(is_same<decltype(v.forward().cat(v.forward())), tuple<int&, int&>>);
        static_assert(is_same<decltype(v.forward().cat(v.move())), tuple<int&, int&&>>);
        static_assert(is_same<decltype(v.move().cat(v.forward())), tuple<int&&, int&>>);
        static_assert(is_same<decltype(v.move().cat(v.move())), tuple<int&&, int&&>>);

        static_assert(is_same<decltype(v.cat(lv)), tuple<int, int>>);
        static_assert(is_same<decltype(v.cat(lv.forward())), tuple<int, int&>>);
        static_assert(is_same<decltype(v.cat(lv.move())), tuple<int, int&&>>);
        static_assert(is_same<decltype(v.forward().cat(lv.forward())), tuple<int&, int&>>);
        static_assert(is_same<decltype(v.forward().cat(lv.move())), tuple<int&, int&&>>);
        static_assert(is_same<decltype(v.move().cat(lv.forward())), tuple<int&&, int&>>);
        static_assert(is_same<decltype(v.move().cat(lv.move())), tuple<int&&, int&&>>);

        /* Lv and Clv */
        static_assert(is_same<decltype(v.cat(clv)), tuple<int, int>>);
        static_assert(is_same<decltype(v.cat(clv.forward())), tuple<int, const int&>>);
        static_assert(is_same<decltype(v.cat(clv.move())), tuple<int, const int&&>>);
        static_assert(is_same<decltype(v.forward().cat(clv.forward())), tuple<int&, const int&>>);
        static_assert(is_same<decltype(v.forward().cat(clv.move())), tuple<int&, const int&&>>);
        static_assert(is_same<decltype(v.move().cat(clv.forward())), tuple<int&&, const int&>>);
        static_assert(is_same<decltype(v.move().cat(clv.move())), tuple<int&&, const int&&>>);

        /* Lv and Rv */
        static_assert(is_same<decltype(v.cat(mov(v))), tuple<int, int>>);
        static_assert(is_same<decltype(v.cat(mov(v).forward())), tuple<int, int&&>>);
        static_assert(is_same<decltype(v.cat(mov(v).move())), tuple<int, int&&>>);
        static_assert(is_same<decltype(v.forward().cat(mov(v).forward())), tuple<int&, int&&>>);
        static_assert(is_same<decltype(v.forward().cat(mov(v).move())), tuple<int&, int&&>>);
        static_assert(is_same<decltype(v.move().cat(mov(v).forward())), tuple<int&&, int&&>>);
        static_assert(is_same<decltype(v.move().cat(mov(v).move())), tuple<int&&, int&&>>);


        /* Clv and Lv */
        static_assert(is_same<decltype(clv.cat(v)), tuple<int, int>>);
        static_assert(is_same<decltype(clv.cat(v.forward())), tuple<int, int&>>);
        static_assert(is_same<decltype(clv.cat(v.move())), tuple<int, int&&>>);
        static_assert(is_same<decltype(clv.forward().cat(v.forward())), tuple<const int&, int&>>);
        static_assert(is_same<decltype(clv.forward().cat(v.move())), tuple<const int&, int&&>>);
        static_assert(is_same<decltype(clv.move().cat(v.forward())), tuple<const int&&, int&>>);
        static_assert(is_same<decltype(clv.move().cat(v.move())), tuple<const int&&, int&&>>);

        static_assert(is_same<decltype(clv.cat(lv)), tuple<int, int>>);
        static_assert(is_same<decltype(clv.cat(lv.forward())), tuple<int, int&>>);
        static_assert(is_same<decltype(clv.cat(lv.move())), tuple<int, int&&>>);
        static_assert(is_same<decltype(clv.forward().cat(lv.forward())), tuple<const int&, int&>>);
        static_assert(is_same<decltype(clv.forward().cat(lv.move())), tuple<const int&, int&&>>);
        static_assert(is_same<decltype(clv.move().cat(lv.forward())), tuple<const int&&, int&>>);
        static_assert(is_same<decltype(clv.move().cat(lv.move())), tuple<const int&&, int&&>>);

        /* Clv and Clv */
        static_assert(is_same<decltype(clv.cat(clv)), tuple<int, int>>);
        static_assert(is_same<decltype(clv.cat(clv.forward())), tuple<int, const int&>>);
        static_assert(is_same<decltype(clv.cat(clv.move())), tuple<int, const int&&>>);
        static_assert(is_same<decltype(clv.forward().cat(clv.forward())), tuple<const int&, const int&>>);
        static_assert(is_same<decltype(clv.forward().cat(clv.move())), tuple<const int&, const int&&>>);
        static_assert(is_same<decltype(clv.move().cat(clv.forward())), tuple<const int&&, const int&>>);
        static_assert(is_same<decltype(clv.move().cat(clv.move())), tuple<const int&&, const int&&>>);

        /* Clv and Rv */
        static_assert(is_same<decltype(clv.cat(mov(v))), tuple<int, int>>);
        static_assert(is_same<decltype(clv.cat(mov(v).forward())), tuple<int, int&&>>);
        static_assert(is_same<decltype(clv.cat(mov(v).move())), tuple<int, int&&>>);
        static_assert(is_same<decltype(clv.forward().cat(mov(v).forward())), tuple<const int&, int&&>>);
        static_assert(is_same<decltype(clv.forward().cat(mov(v).move())), tuple<const int&, int&&>>);
        static_assert(is_same<decltype(clv.move().cat(mov(v).forward())), tuple<const int&&, int&&>>);
        static_assert(is_same<decltype(clv.move().cat(mov(v).move())), tuple<const int&&, int&&>>);

        /* Rv and Lv */
        static_assert(is_same<decltype(mov(v).cat(v)), tuple<int, int>>);
        static_assert(is_same<decltype(mov(v).cat(v.forward())), tuple<int, int&>>);
        static_assert(is_same<decltype(mov(v).cat(v.move())), tuple<int, int&&>>);
        static_assert(is_same<decltype(mov(v).forward().cat(v.forward())), tuple<int&&, int&>>);
        static_assert(is_same<decltype(mov(v).forward().cat(v.move())), tuple<int&&, int&&>>);
        static_assert(is_same<decltype(mov(v).move().cat(v.forward())), tuple<int&&, int&>>);
        static_assert(is_same<decltype(mov(v).move().cat(v.move())), tuple<int&&, int&&>>);

        static_assert(is_same<decltype(mov(v).cat(lv)), tuple<int, int>>);
        static_assert(is_same<decltype(mov(v).cat(lv.forward())), tuple<int, int&>>);
        static_assert(is_same<decltype(mov(v).cat(lv.move())), tuple<int, int&&>>);
        static_assert(is_same<decltype(mov(v).forward().cat(lv.forward())), tuple<int&&, int&>>);
        static_assert(is_same<decltype(mov(v).forward().cat(lv.move())), tuple<int&&, int&&>>);
        static_assert(is_same<decltype(mov(v).move().cat(lv.forward())), tuple<int&&, int&>>);
        static_assert(is_same<decltype(mov(v).move().cat(lv.move())), tuple<int&&, int&&>>);

        /* Rv and Clv */
        static_assert(is_same<decltype(mov(v).cat(clv)), tuple<int, int>>);
        static_assert(is_same<decltype(mov(v).cat(clv.forward())), tuple<int, const int&>>);
        static_assert(is_same<decltype(mov(v).cat(clv.move())), tuple<int, const int&&>>);
        static_assert(is_same<decltype(mov(v).forward().cat(clv.forward())), tuple<int&&, const int&>>);
        static_assert(is_same<decltype(mov(v).forward().cat(clv.move())), tuple<int&&, const int&&>>);
        static_assert(is_same<decltype(mov(v).move().cat(clv.forward())), tuple<int&&, const int&>>);
        static_assert(is_same<decltype(mov(v).move().cat(clv.move())), tuple<int&&, const int&&>>);

        /* Rv and Rv */
        static_assert(is_same<decltype(mov(v).cat(mov(v))), tuple<int, int>>);
        static_assert(is_same<decltype(mov(v).cat(mov(v).forward())), tuple<int, int&&>>);
        static_assert(is_same<decltype(mov(v).cat(mov(v).move())), tuple<int, int&&>>);
        static_assert(is_same<decltype(mov(v).forward().cat(mov(v).forward())), tuple<int&&, int&&>>);
        static_assert(is_same<decltype(mov(v).forward().cat(mov(v).move())), tuple<int&&, int&&>>);
        static_assert(is_same<decltype(mov(v).move().cat(mov(v).forward())), tuple<int&&, int&&>>);
        static_assert(is_same<decltype(mov(v).move().cat(mov(v).move())), tuple<int&&, int&&>>);


        constexpr tuple t1{1, 2};
        constexpr tuple t2{u64(1), 3};

        constexpr auto res = t1.cat(t2);
        static_assert(is_same<decltype(res), const tuple<int, int, u64, int>>);
        static_assert(res[0_c] == 1);
        static_assert(res[1_c] == 2);
        static_assert(res[2_c] == 1);
        static_assert(res[3_c] == 3);

        auto empty_test = tuple{}.cat(tuple{});
        static_assert(empty_test.size() == 0);
        static_assert(is_same<decltype(empty_test), tuple<>>);

        auto empty_test2 = tuple{}.cat(tuple{1});
        static_assert(empty_test2.size() == 1);
        static_assert(is_same<decltype(empty_test2), tuple<int>>);

        empty_test2 = tuple{1}.cat(tuple{});
    }

    SECTION("dispatch") {
        constexpr auto a = tuple{2, 2}.pass_to([](auto&& a, auto&& b) {
            return is_same<decltype(a), int&&> && is_same<decltype(b), int&&>;
        });
        static_assert(a);

        tuple t{2, 2};
        constexpr auto b = t.pass_to([](auto&& a, auto&& b) {
            return is_same<decltype(a), int&> && is_same<decltype(b), int&>;
        });
        static_assert(b);

        constexpr auto c = as_const(t).pass_to([](auto&& a, auto&& b) {
            return is_same<decltype(a), const int&> && is_same<decltype(b), const int&>;
        });
        static_assert(c);

        constexpr auto d = t.move().pass_to([](auto&& a, auto&& b) {
            return is_same<decltype(a), int&&> && is_same<decltype(b), int&&>;
        });
        static_assert(d);

        constexpr auto e = as_const(t).move().pass_to([](auto&& a, auto&& b) {
            return is_same<decltype(a), const int&&> && is_same<decltype(b), const int&&>;
        });
        static_assert(e);
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

    SECTION("reduce") {
        tuple t{123, std::string("hello"), u64(234)};
        auto result = t.reduce(std::string{}, overloaded{
            [](const std::string& sum, const integral auto& v) { return sum + std::to_string(v); },
            [](const std::string& sum, const std::string& v) { return sum + v; },
        });
        REQUIRE(result == "123hello234");

        constexpr tuple t2{100, 200, u64(338)};
        constexpr auto res2 = t2.reduce(0, [](auto sum, auto v) { return sum + v; });
        static_assert(is_same<decltype(res2), const u64>);
        static_assert(res2 == 638);

        tuple t3{std::string("hello"), std::vector<char>{'2', '2', '8'}};

        auto  res3 = mov(t3).reduce(std::string(), [&](auto sum, auto&& v) {
            sum.append(v.data(), v.size());
            auto v2 = mov(v);
            return sum;
        });
        REQUIRE(res3 == "hello228");
        REQUIRE(t3[0_c].empty());
        REQUIRE(t3[1_c].empty());
    }

    SECTION("not moved") {
        struct no_mov_ctor {
            no_mov_ctor() = default;
            no_mov_ctor(no_mov_ctor&& t) = delete;
            no_mov_ctor(const no_mov_ctor& t) = delete;
        };

        tuple<no_mov_ctor> t4;

        /* Ok if compiled */
        mov(t4).foreach([](auto&&) {});

        /* Ok if compiled */
        mov(t4).map([](auto&&) {
            return 1;
        });

        /* Ok if compiled */
        mov(t4).reduce(0, [](auto sum, auto&&) {
            return sum;
        });

        tuple<int, int> t2;

        /* Ok if compiled */
        no_mov_ctor_handler f;
        mov(t2).foreach(mov(f));
        mov(t2).map(mov(f));
        mov(t2).reduce(1, mov(f));
        mov(t2).foreach(no_mov_ctor_handler{});
        mov(t2).map(no_mov_ctor_handler{});
        mov(t2).reduce(1, no_mov_ctor_handler{});
    }
}
