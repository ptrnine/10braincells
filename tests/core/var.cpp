#include <catch2/catch_test_macros.hpp>
#include <core/var.hpp>

using core::none;
using core::none_t;
using core::overloaded;
using core::type;
using core::u32;
using core::u64;
using core::u8;
using core::var;
using core::visit;
using core::is_same;

struct nontriv_dtor {
    bool& called;
    nontriv_dtor(bool& icalled): called(icalled) {
        called = false;
    }
    ~nontriv_dtor() {
        called = true;
    }
};

struct var1_sz {
    union {
        u32 a;
        float b;
        char c;
        bool d;
    };
    u8 i;
};

struct var2_sz {
    union {
        nontriv_dtor a0;
        u32 a;
        float b;
        char c;
        bool d;
    };
    u8 i;
};

struct throw_move_ctor {
    u64 some_data;
    u64 some_data2;
    throw_move_ctor(throw_move_ctor&&) {}
};

struct var3_sz {
    union {
        throw_move_ctor a;
        u32 b;
        float c;
        char d;
        bool e;
    } a;
    union {
        throw_move_ctor a;
        u32 b;
        float c;
        char d;
        bool e;
    } b;
    u8 i;
};

struct nontriv_dtor_throw_move_ctor {
    char a[228];
    ~nontriv_dtor_throw_move_ctor() {}
    nontriv_dtor_throw_move_ctor(nontriv_dtor_throw_move_ctor&&) {}
};

struct var4_sz {
    union {
        nontriv_dtor a;
        throw_move_ctor b;
        nontriv_dtor_throw_move_ctor c;
    } a;
    union {
        nontriv_dtor a;
        throw_move_ctor b;
        nontriv_dtor_throw_move_ctor c;
    } b;
    u8 i;
};

struct test_type {
    int a;
    bool b;
    double c;
    constexpr test_type(auto ia, auto ib, auto ic): a(ia), b(ib), c(ic) {}
};


TEST_CASE("var") {
    SECTION("types") {
        static_assert(sizeof(var<u32, float, char, bool>) == sizeof(var1_sz));
        static_assert(sizeof(var<nontriv_dtor, u32, float, char, bool>) == sizeof(var2_sz));
        static_assert(sizeof(var<throw_move_ctor, u32, float, char, bool>) == sizeof(var3_sz));
        static_assert(sizeof(var<nontriv_dtor, throw_move_ctor, nontriv_dtor_throw_move_ctor>) == sizeof(var4_sz));
    }

    SECTION("ctor") {
        constexpr var<int, float, bool> a{2289};
        static_assert(a.get<int>() == 2289);

        constexpr var<int, float, bool> b{13.37f};
        static_assert(b.get<float>() == 13.37f);

        constexpr var<int, float, test_type, bool> c{type<test_type>, 123, true, 1337.1337};
        static_assert(c.get<test_type>().a == 123);
        static_assert(c.get<test_type>().b == true);
        static_assert(c.get<test_type>().c == 1337.1337);

        constexpr var<int, var<test_type, float>, bool> d = var<test_type, float>{type<float>, 2345.23f};
        static_assert(d.get(type<var<test_type, float>>).get<float>() == 2345.23f);
    }

    SECTION("var bad access") {
        var<int, float, bool> a{2};

        bool thrown = false;
        try {
            a.get<float>();
        } catch (const core::bad_var_access&) {
            thrown = true;
        }
        REQUIRE(thrown);
    }

    SECTION("never valueless") {
        struct dtor_called {
            dtor_called(bool& v): called(v) {}
            ~dtor_called() {
                called = true;
            }
            bool& called;
        };

        struct test {
            test() = default;
            test(test&&) {
                throw 1;
            }
            test& operator=(test&&) {
                throw 1;
            }
            operator int() {
                throw 1;
            }
        };

        bool is_dtor_called = false;
        var<int, float, dtor_called> a{type<dtor_called>, is_dtor_called};
        try {
            a.emplace<int>(test{});
        }
        catch (...) {
        }

        a.get<dtor_called>();
        REQUIRE(!is_dtor_called);

        static_assert(!a.valueless_by_exception());
    }

    SECTION("trivial default ctor") {
        struct triv_ctor {
        };

        var<int, float, bool, double, triv_ctor> a{type<triv_ctor>};
        a.get<triv_ctor>();

        a.emplace<int>(1);
        REQUIRE(a.get<int>() == 1);

        a.emplace<triv_ctor>();
        a.get<triv_ctor>();
    }

    SECTION("non-trivial default ctor") {
        struct nontriv_ctor {
            nontriv_ctor(bool& called) {
                called = true;
            }
        };
        bool called = false;
        var<int, nontriv_ctor> a{type<nontriv_ctor>, called};
        REQUIRE(called);
        a.get<nontriv_ctor>();

        a.emplace<int>(123);
        REQUIRE(a.get<int>() == 123);

        called = false;
        a.emplace<nontriv_ctor>(called);
        REQUIRE(called);
        a.get<nontriv_ctor>();
    }

    SECTION("non-default ctor") {
        struct test_ctor {
            test_ctor(int, float, const char(&)[6]) {}
        };
        var<test_ctor, float, int> b{type<test_ctor>, 1234, 12.23f, "hello"};
        b.get<test_ctor>();

        b = 1.f;
        REQUIRE(b.get<float>() == 1.f);

        b.emplace<test_ctor>(11, 1.1f, "qwewe");
        b.get<test_ctor>();
    }

    SECTION("dtor") {
        struct test_dtor {
            bool& called;
            test_dtor(bool& v) noexcept: called(v) {}
            ~test_dtor() {
                called = true;
            }
        };
        bool called = false;
        var<test_dtor, int> a{type<test_dtor>, called};

        REQUIRE(!called);
        a = 1;
        REQUIRE(called);

        called = false;
        a.emplace<test_dtor>(called);
        REQUIRE(!called);
        a.emplace<int>(1);
        REQUIRE(called);

        called = false;
        {
            var<test_dtor, float> b(type<test_dtor>, called);
            REQUIRE(!called);
        }
        REQUIRE(called);

        struct test_dtor2 {
            bool& called;
            test_dtor2(bool& v): called(v) {}
            ~test_dtor2() {
                called = true;
            }
        };

        var<test_dtor2, int> b(type<int>);
        called = false;
        b.emplace<test_dtor2>(called);
        /* Called in tmp in var implementation */
        REQUIRE(called);
    }

    SECTION("visit") {
        var<u32, u64> v1 = u32(23);
        var<bool, const char*> v2 = "hello";

        auto r1 = visit(v1, v2, overloaded{[](auto a, auto b) {
            return is_same<decltype(a), u32> && is_same<decltype(b), const char*>;
        }});
        REQUIRE(r1);

        constexpr auto r2 = visit(var<float, int>{2}, [](auto v) { return is_same<decltype(v), int>; });
        static_assert(r2);
    }
}
