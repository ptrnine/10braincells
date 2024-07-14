#include <catch2/catch_test_macros.hpp>
#include "core/function.hpp"

using namespace core;

void test_function(int& var_to_set, int value) {
    var_to_set = value;
}

void test_function_noexcept(int& var_to_set, int value) noexcept {
    var_to_set = value;
}

auto test_sum(auto a, auto b) {
    return a + b;
}

TEST_CASE("function") {
    SECTION("void() call") {
        bool called = false;

        function<void(), 16> f = [&] {
            called = true;
        };
        f();
        REQUIRE(called);
    }

    SECTION("stateless function") {
        function<void(), sizeof([]{})> f;
        REQUIRE(!f);
        auto f2 = mov(f);
        REQUIRE(!f);
        REQUIRE(!f2);

        function f3 = [] {};
        REQUIRE(f3);
        f3 = f2;
        REQUIRE(!f3);
    }

    SECTION("reset") {
        function f = []{};
        REQUIRE(f);
        f = decltype(f){};
        REQUIRE(!f);

        f = []{};
        REQUIRE(f);
        f = null;
        REQUIRE(!f);

        f = []{};
        REQUIRE(f);
        f.reset();
        REQUIRE(!f);
    }

    SECTION("emplace") {
        struct test {
            test(int n, float f, const char* m, int& c, int& d): num(n), fl(f), msg(m), called(c), dtor(d) {}
            test(const test&) = delete;
            test(test&&) = delete;
            void operator()(int v) {
                switch (v) {
                case 0:
                    REQUIRE(num == 1);
                    REQUIRE(fl == 2.234f);
                    REQUIRE(std::string(msg) == "messssssage");
                    break;
                default:
                    REQUIRE(num == 222);
                    REQUIRE(fl == 1.123f);
                    REQUIRE(std::string(msg) == "test");
                }
                ++called;
            }

            ~test() {
                ++dtor;
            }

            int num;
            float fl;
            const char* msg;
            int& called;
            int& dtor;
        };

        int dtor_called = 0;
        {
            int called = 0;
            function<void(int), 40> f(type<test>, 1, 2.234f, "messssssage", called, dtor_called);
            f(0);
            REQUIRE(called == 1);

            f.emplace<test>(222, 1.123f, "test", called, dtor_called);
            REQUIRE(dtor_called == 1);
            f(1);
            REQUIRE(called == 2);
        }
        REQUIRE(dtor_called == 2);
    }

    SECTION("deleted copy ctor") {
        struct test {
            test() = default;
            test(const test&) = delete;
            test(test&&) = default;
            void operator()() {}
        };

        function<void(), 16> f = test();
        function<void(), 16> f2;
        CHECK_THROWS_AS(f2 = f, function_copy_ctor_error);
        CHECK_NOTHROW(f2 = mov(f));
    }

    SECTION("deleted move ctor") {
        struct test {
            test(int& cctor_called, int& dtor_c): cctor_ok(&cctor_called), dtor(&dtor_c) {}
            test(const test& c): cctor_ok(nullptr), dtor(nullptr) {
                if (c.cctor_ok)
                    ++(*c.cctor_ok);
            }
            test(test&&) = delete;
            ~test() {
                if (dtor)
                    ++(*dtor);
            }
            void operator()() {}

            int* cctor_ok;
            int* dtor;
        };

        int cctor_called = 0;
        int dtor_called  = 0;
        {
            function<void(), 24> f(type<test>, cctor_called, dtor_called);
            function<void(), 24> f2;
            CHECK_NOTHROW(f2 = f);
            REQUIRE(cctor_called == 1);
            REQUIRE(dtor_called == 0);

            f.emplace<test>(cctor_called, dtor_called);
            REQUIRE(cctor_called == 1);
            /* Was called on emplace */
            REQUIRE(dtor_called == 1);

            CHECK_NOTHROW(f2 = mov(f));
            REQUIRE(!bool(f));
            REQUIRE(cctor_called == 2);
            REQUIRE(dtor_called == 1);
        }
        REQUIRE(dtor_called == 2);
    }

    SECTION("plain function") {
        int n = 0;
        function f = test_function;

        f(n, 1337);
        REQUIRE(n == 1337);

        f = test_function_noexcept;
        f(n, 123456);
        REQUIRE(n == 123456);
    }

    SECTION("member function") {
        struct test {
            void set(int v) {
                value = v;
            }
            void set1(int v) noexcept {
                value = v;
            }
            int get() const noexcept {
                return value;
            }
            int get1() const {
                return value;
            }
            int value = 0;
        };

        test t;

        function set = &test::set;
        function get = &test::get;
        function set1 = &test::set1;
        function get1 = &test::get1;
        set = &test::set;
        get = &test::get;
        set1 = &test::set1;
        get1 = &test::get1;

        set(t, 100200300);
        REQUIRE(get1(t) == 100200300);
        set1(t, 1337);
        REQUIRE(get(t) == 1337);
    }

    SECTION("virtual member function") {
        struct base {
            virtual int lol() const = 0;
            virtual ~base() = default;
        };

        struct derived1 : public base {
            int lol() const override {
                return 13371337;
            }
        };

        struct derived2 : public base {
            int lol() const override {
                return 66666666;
            }
        };

        base* o1 = new derived1();
        base* o2 = new derived2();

        function f = &base::lol;
        REQUIRE(f(*o1) == 13371337);
        REQUIRE(f(*o2) == 66666666);

        delete o1;
        delete o2;
    }

    SECTION("lambda type deduction") {
        function f1 = [](int a) { return a; };
        function f2 = [](int a) mutable { return a; };
        function f3 = [](int a) mutable noexcept { return a; };
        function f4 = [](int a) noexcept { return a; };

        REQUIRE(f1(1234) == 1234);
        REQUIRE(f2(1235) == 1235);
        REQUIRE(f3(1236) == 1236);
        REQUIRE(f4(1237) == 1237);
    }

    SECTION("template functions") {
        auto l = [](auto v) { return v; };
        function<int(int), 8> f1 = l;
        function<float(float), 8> f2 = l;
        function<i64(int), 8> f3 = l;

        REQUIRE(f1(123) == 123);
        REQUIRE(f2(123.123f) == 123.123f);
        REQUIRE(f3(1234) == 1234);

        function<float(int, float), 8> f4 = test_sum;
        function<i64(int, i64), 8> f5 = test_sum;
        REQUIRE(f4(1, 1.5f) < 2.5001);
        REQUIRE(f4(1, 1.5f) > 2.4999);
        REQUIRE(f5(200, 56) == 256);
    }
}
