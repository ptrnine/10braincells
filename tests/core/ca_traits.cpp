#include <catch2/catch_test_macros.hpp>
#include <core/traits/ca_traits.hpp>

using namespace core;

struct traits_state {
    static traits_state& instance() {
        static traits_state st;
        return st;
    }

    void reset() {
        cc = false;
        mc = false;
        ca = false;
        ma = false;
    }

    bool cc = false;
    bool mc = false;
    bool ca = false;
    bool ma = false;
};

inline traits_state& state() {
    return traits_state::instance();
}

struct traits {
    static void cc(auto&&, auto&&) {
        state().cc = true;
    }
    static void mc(auto&&, auto&&) {
        state().mc = true;
    }
    static auto& ca(auto& it, auto&&) {
        state().ca = true;
        return it;
    }
    static auto& ma(auto& it, auto&&) {
        state().ma = true;
        return it;
    }
};


struct traits_protected_access {
    static void cc(auto& it, auto&& rhs) {
        it._protected = rhs._protected;
    }
};

struct base {};

template <typename T>
auto test_cc() {
    T v1;
    [[maybe_unused]] T v2 = v1;
}

template <typename T>
void test_mc() {
    T v1;
    [[maybe_unused]] T v2 = mov(v1);
}

template <typename T>
void test_ca() {
    T v1;
    T v2;
    v1 = v2;
}

template <typename T>
void test_ma() {
    T v1;
    T v2;
    v1 = mov(v2);
}

template <typename T>
void test_all() {
    test_cc<T>();
    test_mc<T>();
    test_ca<T>();
    test_ma<T>();
}

TEST_CASE("ca_traits") {
    SECTION("all trivial") {
        state().reset();
        struct test {};
        test_all<ca_traits<traits, base, test>>();

        CHECK(!state().cc);
        CHECK(!state().mc);
        CHECK(!state().ca);
        CHECK(!state().ma);
    }
    SECTION("cc") {
        state().reset();
        struct test {
            test() = default;
            test(const test&) {}
            test& operator=(const test&) = default;
            test(test&&) = default;
            test& operator=(test&&) = default;
        };
        test_all<ca_traits<traits, base, test>>();

        CHECK(state().cc);
        CHECK(!state().mc);
        CHECK(!state().ca);
        CHECK(!state().ma);
    }
    SECTION("mc") {
        state().reset();
        struct test {
            test() = default;
            test(const test&) = default;
            test& operator=(const test&) = default;
            test(test&&) noexcept {}
            test& operator=(test&&) = default;
        };
        test_all<ca_traits<traits, base, test>>();

        CHECK(!state().cc);
        CHECK(state().mc);
        CHECK(!state().ca);
        CHECK(!state().ma);
    }
    SECTION("ca") {
        state().reset();
        struct test {
            test() = default;
            test(const test&) = default;
            test& operator=(const test&) {
                return *this;
            }
            test(test&&) = default;
            test& operator=(test&&) = default;
        };
        test_all<ca_traits<traits, base, test>>();

        CHECK(!state().cc);
        CHECK(!state().mc);
        CHECK(state().ca);
        CHECK(!state().ma);
    }
    SECTION("ca") {
        state().reset();
        struct test {
            test() = default;
            test(const test&) = default;
            test& operator=(const test&) = default;
            test(test&&) = default;
            test& operator=(test&&) noexcept {
                return *this;
            }
        };
        test_all<ca_traits<traits, base, test>>();

        CHECK(!state().cc);
        CHECK(!state().mc);
        CHECK(!state().ca);
        CHECK(state().ma);
    }
    SECTION("protected access") {
        class base {
        public:
            void set(int v) {
                _protected = v;
            }

            int get() const {
                return _protected;
            }

        protected:
            int _protected;
        };
        struct test {
            test() = default;
            test(const test&) {}
            test& operator=(const test&) = default;
            test(test&&) = default;
            test& operator=(test&&) = default;
        };

        ca_traits<traits_protected_access, base, test> a;
        a.set(228);
        decltype(a) b = a;
        CHECK(b.get() == 228);
    }
    SECTION("noexcept default") {
        struct test1 {};
        struct base {};
        using t1 = ca_traits<traits, base, test1>;
        static_assert(nothrow_copy_ctor<t1>);
        static_assert(nothrow_move_ctor<t1>);
        static_assert(nothrow_copy_assign<t1>);
        static_assert(nothrow_move_assign<t1>);

        struct test2 {
            test2() = default;
            test2(const test2&) noexcept = default;
            test2& operator=(const test2&) noexcept(false) = default;
            test2(test2&&) noexcept(false) = default;
            test2& operator=(test2&&) noexcept(false) = default;
        };
        using t2 = ca_traits<traits, base, test2>;
        static_assert(nothrow_copy_ctor<t2>);
        static_assert(!nothrow_move_ctor<t2>);
        static_assert(!nothrow_copy_assign<t2>);
        static_assert(!nothrow_move_assign<t2>);

        struct test3 {
            test3() = default;
            test3(const test3&) noexcept(false) = default;
            test3& operator=(const test3&) noexcept = default;
            test3(test3&&) noexcept(false) = default;
            test3& operator=(test3&&) noexcept(false) = default;
        };
        using t3 = ca_traits<traits, base, test3>;
        static_assert(!nothrow_copy_ctor<t3>);
        static_assert(!nothrow_move_ctor<t3>);
        static_assert(nothrow_copy_assign<t3>);
        static_assert(!nothrow_move_assign<t3>);

        struct test4 {
            test4() = default;
            test4(const test4&) noexcept(false) = default;
            test4& operator=(const test4&) noexcept(false) = default;
            test4(test4&&) noexcept = default;
            test4& operator=(test4&&) noexcept(false) = default;
        };
        using t4 = ca_traits<traits, base, test4>;
        static_assert(!nothrow_copy_ctor<t4>);
        static_assert(nothrow_move_ctor<t4>);
        static_assert(!nothrow_copy_assign<t4>);
        static_assert(!nothrow_move_assign<t4>);

        struct test5 {
            test5() = default;
            test5(const test5&) noexcept(false) = default;
            test5& operator=(const test5&) noexcept(false) = default;
            test5(test5&&) noexcept(false) = default;
            test5& operator=(test5&&) noexcept = default;
        };
        using t5 = ca_traits<traits, base, test5>;
        static_assert(!nothrow_copy_ctor<t5>);
        static_assert(!nothrow_move_ctor<t5>);
        static_assert(!nothrow_copy_assign<t5>);
        static_assert(nothrow_move_assign<t5>);
    }

    SECTION("noexcept non-default") {
        struct test1 {
            test1() {}
            test1(const test1&) noexcept {}
            test1& operator=(const test1&) noexcept {
                return *this;
            }
            test1(test1&&) noexcept {}
            test1& operator=(test1&&) noexcept {
                return *this;
            }
        };
        struct base {};
        using t1 = ca_traits<traits, base, test1>;
        static_assert(nothrow_copy_ctor<t1>);
        static_assert(nothrow_move_ctor<t1>);
        static_assert(nothrow_copy_assign<t1>);
        static_assert(nothrow_move_assign<t1>);

        struct test2 {
            test2() {}
            test2(const test2&) noexcept {}
            test2& operator=(const test2&) noexcept(false) {
                return *this;
            }
            test2(test2&&) noexcept(false) {}
            test2& operator=(test2&&) noexcept(false) {
                return *this;
            }
        };
        using t2 = ca_traits<traits, base, test2>;
        static_assert(nothrow_copy_ctor<t2>);
        static_assert(!nothrow_move_ctor<t2>);
        static_assert(!nothrow_copy_assign<t2>);
        static_assert(!nothrow_move_assign<t2>);

        struct test3 {
            test3() {}
            test3(const test3&) noexcept(false) {}
            test3& operator=(const test3&) noexcept {
                return *this;
            }
            test3(test3&&) noexcept(false) {}
            test3& operator=(test3&&) noexcept(false) {
                return *this;
            }
        };
        using t3 = ca_traits<traits, base, test3>;
        static_assert(!nothrow_copy_ctor<t3>);
        static_assert(!nothrow_move_ctor<t3>);
        static_assert(nothrow_copy_assign<t3>);
        static_assert(!nothrow_move_assign<t3>);

        struct test4 {
            test4() {}
            test4(const test4&) noexcept(false) {}
            test4& operator=(const test4&) noexcept(false) {
                return *this;
            }
            test4(test4&&) noexcept {}
            test4& operator=(test4&&) noexcept(false) {
                return *this;
            }
        };
        using t4 = ca_traits<traits, base, test4>;
        static_assert(!nothrow_copy_ctor<t4>);
        static_assert(nothrow_move_ctor<t4>);
        static_assert(!nothrow_copy_assign<t4>);
        static_assert(!nothrow_move_assign<t4>);

        struct test5 {
            test5() {}
            test5(const test5&) noexcept(false) {}
            test5& operator=(const test5&) noexcept(false) {
                return *this;
            }
            test5(test5&&) noexcept(false) {}
            test5& operator=(test5&&) noexcept {
                return *this;
            }
        };
        using t5 = ca_traits<traits, base, test5>;
        static_assert(!nothrow_copy_ctor<t5>);
        static_assert(!nothrow_move_ctor<t5>);
        static_assert(!nothrow_copy_assign<t5>);
        static_assert(nothrow_move_assign<t5>);
    }
}
