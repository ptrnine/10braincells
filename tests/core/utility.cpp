#include <catch2/catch_test_macros.hpp>

#include "core/utility/idx_dispatch.hpp"
#include "core/utility/idx_dispatch_r.hpp"
using core::idx_dispatch;
using core::idx_dispatch_r;
using core::int_const;

TEST_CASE("idx_dispatch") {
    SECTION("dispatch test 1") {
        size_t called = 0;
        idx_dispatch<20>(18, [&](auto n) {
            ++called;
            REQUIRE(n == 18);
        });
        REQUIRE(called == 1);
    }
    SECTION("dispatch test 2") {
        size_t called = 0;
        idx_dispatch<17>(11, [&](auto n) {
            ++called;
            REQUIRE(n == 11);
        });
        REQUIRE(called == 1);
    }
    SECTION("return same types") {
        auto f = [](auto n) {
            if constexpr (n == 4)
                return "ok";
            else
                return "fail";
        };
        REQUIRE(std::string(idx_dispatch<10>(4, f)) == "ok");
        REQUIRE(std::string(idx_dispatch<10>(3, f)) == "fail");
    }
    SECTION("return different types") {
        auto f = [](auto n) {
            if constexpr (n == 4)
                return "four";
            else
                return std::string("some number");
        };

        REQUIRE(idx_dispatch_r<std::string, 10>(4, f) == "four");
        REQUIRE(idx_dispatch_r<std::string, 10>(5, f) == "some number");
    }
    SECTION("constexpr") {
        struct test {
            constexpr size_t f(size_t idx) {
                return idx_dispatch<10>(idx, [](auto v) {
                    if constexpr (v > 5)
                        return v * 2;
                    else
                        return v / 2;
                });
            }
        };

        test t;
        constexpr auto v1 = t.f(4);
        constexpr auto v2 = t.f(8);
        static_assert(v1 == 2);
        static_assert(v2 == 16);
    }
}

#include <array>
#include "core/utility/int_seq.hpp"
using core::int_seq;
using core::idx_seq;
using core::make_int_seq;
using core::make_idx_seq;

TEST_CASE("int_seq") {
    SECTION("int") {
        []<int... Ns>(int_seq<int, Ns...>) {
            static_assert(sizeof...(Ns) == 5);
            static_assert(std::array{Ns...} == std::array{0, 1, 2, 3, 4});
        }(make_int_seq<int, 5>());
    }
    SECTION("size_t") {
        []<size_t... Ns>(int_seq<size_t, Ns...>) {
            static_assert(sizeof...(Ns) == 5);
            static_assert(std::array<size_t, 5>{Ns...} == std::array<size_t, 5>{0, 1, 2, 3, 4});
        }(make_idx_seq<5>());
    }
}

#include "core/utility/move.hpp"
using core::mov;

TEST_CASE("move") {
    enum class pass_type { lv, clv, rv, crv };

    struct pass_test {
        constexpr pass_type f(int&) const {
            return pass_type::lv;
        }
        constexpr pass_type f(const int&) const {
            return pass_type::clv;
        }
        constexpr pass_type f(int&&) const {
            return pass_type::rv;
        }
        constexpr pass_type f(const int&&) const {
            return pass_type::crv;
        }
    };

    pass_test t;

    int v = 0;
    const int cv = 0;

    static_assert(t.f(v) == pass_type::lv);
    static_assert(t.f(cv) == pass_type::clv);
    static_assert(t.f(1) == pass_type::rv);

    static_assert(t.f(mov(v)) == pass_type::rv);
    static_assert(t.f(mov(cv)) == pass_type::crv);
    static_assert(t.f(mov(1)) == pass_type::rv);
}

#include "core/traits/is_same.hpp"
#include "core/utility/forward.hpp"
using core::fwd;
using core::is_same;

#define FWD(what) static_cast<decltype(what)>(what)

TEST_CASE("forward") {
    enum class value_type { lv, clv, rv, crv, v };

    constexpr auto test = [](auto&& v) {
        if constexpr (is_same<decltype(v), int&>)
            return value_type::lv;
        else if constexpr (is_same<decltype(v), const int&>)
            return value_type::clv;
        else if constexpr (is_same<decltype(v), int&&>)
            return value_type::rv;
        else if constexpr (is_same<decltype(v), const int&&>)
            return value_type::crv;
        else if constexpr (is_same<decltype(v), int>)
            return value_type::v;
    };

    constexpr auto test_forward = []<typename T>(auto test, T&& v) {
        return test(fwd<T>(v));
    };
    constexpr auto test_std_forward = []<typename T>(auto test, T&& v) {
        return test(std::forward<T>(v));
    };
    constexpr auto test_fwd = [](auto test, auto&& v) {
        return test(FWD(v));
    };
    constexpr auto no_forward = [](auto test, auto&& v) {
        return test(v);
    };

    int value = 0;
    const int cv = 0;

    static_assert(test_forward(test, value) == value_type::lv);
    static_assert(test_forward(test, cv) == value_type::clv);
    static_assert(test_forward(test, mov(value)) == value_type::rv);
    static_assert(test_forward(test, mov(cv)) == value_type::crv);
    static_assert(test_forward(test, 1) == value_type::rv);

    static_assert(test_std_forward(test, value) == value_type::lv);
    static_assert(test_std_forward(test, cv) == value_type::clv);
    static_assert(test_std_forward(test, mov(value)) == value_type::rv);
    static_assert(test_std_forward(test, mov(cv)) == value_type::crv);
    static_assert(test_std_forward(test, 1) == value_type::rv);

    static_assert(test_fwd(test, value) == value_type::lv);
    static_assert(test_fwd(test, cv) == value_type::clv);
    static_assert(test_fwd(test, mov(value)) == value_type::rv);
    static_assert(test_fwd(test, mov(cv)) == value_type::crv);
    static_assert(test_fwd(test, 1) == value_type::rv);

    static_assert(no_forward(test, value) == value_type::lv);
    static_assert(no_forward(test, cv) == value_type::clv);
    static_assert(no_forward(test, mov(value)) == value_type::lv);
    static_assert(no_forward(test, mov(cv)) == value_type::clv);
    static_assert(no_forward(test, 1) == value_type::lv);
}

#include "core/utility/as_const.hpp"
using core::as_const;

TEST_CASE("as_const") {
    int a = 0;
    static_assert(is_same<decltype(as_const(a)), const int&>);
}
