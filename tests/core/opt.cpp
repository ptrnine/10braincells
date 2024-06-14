#include <catch2/catch_test_macros.hpp>
#include <core/opt.hpp>
#include <core/traits/is_same.hpp>
#include <core/utility/as_const.hpp>

#include "move_copy_assign_ctor.hpp"

using core::opt;
using core::null;
using core::init;
using core::as_const;
using core::is_same;

TEST_CASE("opt") {
    SECTION("ctor and assign") {
        MOVE_COPY_IMPLICITLY_DELETED_MOVE_TEST(opt, assign_ctor_state{}, [](auto&& v) -> auto& { return *v; });

        struct test {
            test(int, float, const char*): ctor_called(true) {}
            test(test&& rhs) noexcept: mctor_called(true) {
                rhs.moved = true;
            }
            test& operator=(test&& rhs) noexcept {
                rhs.moved = true;
                return *this;
            }
            bool ctor_called = false;
            bool mctor_called = false;
            bool moved = false;
        };

        opt<test> a{init, 2, 2.2f, "hello"};
        REQUIRE(a.has_value());
        REQUIRE(a->ctor_called);
        a.reset();
        REQUIRE(!a.has_value());

        a = test{1, 3.f, ""};
        REQUIRE(a.has_value());
        REQUIRE(!a->ctor_called);
        REQUIRE(a->mctor_called);

        opt b = std::move(a);
        static_assert(core::is_same<decltype(b), opt<test>>);
        REQUIRE(a.has_value());
        CHECK(a->moved);
        REQUIRE(b.has_value());
        CHECK(!b->moved);

        opt c = 2;
        static_assert(core::is_same<decltype(c), opt<int>>);
    }

    SECTION("null") {
        opt<int> a = null;
        opt<int> b = nullptr;
        CHECK(a.empty());
        CHECK(b.empty());

        a = 2;
        b = 3;
        REQUIRE(a.has_value());
        REQUIRE(b.has_value());
        CHECK(*a == 2);
        CHECK(*b == 3);

        a = null;
        b = nullptr;
        CHECK(a.empty());
        CHECK(b.empty());
    }

    SECTION("value") {
        opt o1 = 2323;
        static_assert(core::is_same<decltype(o1.value()), int&>);
        static_assert(core::is_same<decltype(as_const(o1).value()), const int&>);
        static_assert(core::is_same<decltype(mov(o1).value()), int&&>);
        static_assert(core::is_same<decltype(mov(as_const(o1)).value()), const int&&>);
        REQUIRE(o1.has_value());
        REQUIRE(o1.value() == 2323);
    }

    SECTION("value_or") {
        struct test {
            test() = default;
            test(const test&) = default;
            test& operator=(const test&) = default;
            test(test&& t) noexcept {
                t.moved = true;
            }
            test& operator=(test&& t) noexcept {
                t.moved = true;
                return* this;
            }
            bool moved = false;
        };

        opt o1{test{}};
        CHECK(!o1->moved);

        auto r1 = o1.value_or(test{});
        CHECK(!r1.moved);
        CHECK(!o1->moved);

        auto r2 = mov(o1).value_or(test{});
        CHECK(!r2.moved);
        CHECK(o1->moved);

        static_assert(opt{228}.value_or(1337) == 228);
        static_assert(opt<int>{}.value_or(1337) == 1337);
    }

    SECTION("and_then") {
        auto r1 = opt{1}.and_then([](int v) {
            return opt{std::to_string(v)};
        });
        REQUIRE(r1.has_value());
        CHECK(r1.get() == "1");

        opt<std::string> o1{"228"};
        auto r2 = mov(o1).and_then([](std::string&& s) {
            return opt{std::stol(s)};
        });
        REQUIRE(o1.has_value());
        CHECK(o1.get() == "228");
        REQUIRE(r2.has_value());
        CHECK(r2.get() == 228);

        opt<std::string> o2{"228"};
        auto r3 = mov(o2).and_then([](std::string&& s) {
            std::string tmp{core::mov(s)};
            return opt{std::stol(tmp)};
        });
        REQUIRE(o2.has_value());
        CHECK(o2.get().empty());
        REQUIRE(r3.has_value());
        CHECK(r3.get() == 228);

        auto r4 = opt<int>().and_then([](auto&&) {
            return opt<std::string>("hello");
        });
        REQUIRE(r4.empty());

        /* Check overloads */
        opt o = 229;
        bool ok = false;
        bool ok2 = true;
        o.and_then([&](auto&& v) {
            if constexpr (is_same<decltype(v), int&>)
                ok = true;
            else
                ok2 = false;
            return opt<const char*>{};
        });
        CHECK((ok && ok2));

        ok = false;
        ok2 = true;
        as_const(o).and_then([&](auto&& v) {
            if constexpr (is_same<decltype(v), const int&>)
                ok = true;
            else
                ok2 = false;
            return opt<const char*>{};
        });
        CHECK((ok && ok2));

        ok = false;
        ok2 = true;
        mov(as_const(o)).and_then([&](auto&& v) {
            if constexpr (is_same<decltype(v), const int&&>)
                ok = true;
            else
                ok2 = false;
            return opt<const char*>{};
        });
        CHECK((ok && ok2));

        ok = false;
        ok2 = true;
        mov(o).and_then([&](auto&& v) {
            if constexpr (is_same<decltype(v), int&&>)
                ok = true;
            else
                ok2 = false;
            return opt<const char*>{};
        });
        CHECK((ok && ok2));
    }

    SECTION("or_else") {
        auto r1 = opt{1}.or_else([]{ return opt{2}; });
        REQUIRE(r1.has_value());
        CHECK(r1.get() == 1);

        auto r2 = opt<int>{}.or_else([]{ return opt{2}; });
        REQUIRE(r2.has_value());
        CHECK(r2.get() == 2);
    }

    SECTION("map/transform") {
        auto r1 = opt{1337}.map(static_cast<std::string(*)(int)>(std::to_string));
        REQUIRE(r1.has_value());
        CHECK(*r1 == "1337");

        /* Check overloads */
        opt o = 229;
        bool ok = false;
        bool ok2 = true;
        o.map([&](auto&& v) {
            if constexpr (is_same<decltype(v), int&>)
                ok = true;
            else
                ok2 = false;
            return 2;
        });
        CHECK((ok && ok2));

        ok = false;
        ok2 = true;
        as_const(o).map([&](auto&& v) {
            if constexpr (is_same<decltype(v), const int&>)
                ok = true;
            else
                ok2 = false;
            return 2;
        });
        CHECK((ok && ok2));

        ok = false;
        ok2 = true;
        mov(as_const(o)).map([&](auto&& v) {
            if constexpr (is_same<decltype(v), const int&&>)
                ok = true;
            else
                ok2 = false;
            return 2;
        });
        CHECK((ok && ok2));

        ok = false;
        ok2 = true;
        mov(o).map([&](auto&& v) {
            if constexpr (is_same<decltype(v), int&&>)
                ok = true;
            else
                ok2 = false;
            return 2;
        });
        CHECK((ok && ok2));
    }

    SECTION("foreach") {
        bool pass = false;
        for (auto&& v : opt{228}) {
            static_assert(is_same<decltype(v), int&>);
            CHECK(v == 228);
            pass = true;
        }
        CHECK(pass);

        pass = false;
        opt o{1337};
        for (auto&& v : as_const(o)) {
            static_assert(is_same<decltype(v), const int&>);
            CHECK(v == 1337);
            pass = true;
        }
        CHECK(pass);

        for (auto&& v : opt<int>{}) {
            static_assert(is_same<decltype(v), int&>);
            pass = false;
        }
        CHECK(pass);
    }
}
