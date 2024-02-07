#include <concepts>
#include <catch2/catch_test_macros.hpp>

/*
 * any_of
 */

#include "core/nostd/concepts/any_of.hpp"
using core::any_of;

template <typename T>
struct any_of_test;

template <any_of<int, double> T>
struct any_of_test<T> {
    constexpr int test() {
        return 0;
    }
};

template <any_of<float, const char*> T>
struct any_of_test<T> {
    constexpr int test() {
        return 1;
    }
};

TEST_CASE("any_of") {
    static_assert(any_of<int, void, float, int>);
    static_assert(!any_of<double, void, float, int>);
    static_assert(!any_of<int>);
    static_assert(any_of_test<int>().test() == 0);
    static_assert(any_of_test<double>().test() == 0);
    static_assert(any_of_test<float>().test() == 1);
    static_assert(any_of_test<const char*>().test() == 1);
}

/*
 * assign, copy_assign, move_assign
 */

#include "core/nostd/concepts/assign.hpp"
using core::assign;
using core::copy_assign;
using core::move_assign;

struct copy_assign_test {
    copy_assign_test& operator=(const copy_assign_test&) = default;
    copy_assign_test& operator=(copy_assign_test&&) = delete;
};

struct move_assign_test {
    move_assign_test& operator=(const move_assign_test&) = delete;
    move_assign_test& operator=(move_assign_test&&) = default;
};

struct no_assign_test {
    no_assign_test& operator=(const no_assign_test&) = delete;
};

struct both_assign_test {};

struct some_struct {};

struct some_assign_test {
    some_assign_test& operator=(const some_struct&) {
        return *this;
    }
};

TEST_CASE("assign") {
    /*
     * std
     */
    static_assert(!std::is_assignable_v<copy_assign_test, copy_assign_test>);
    static_assert(std::is_assignable_v<copy_assign_test, copy_assign_test&>);
    static_assert(!std::is_assignable_v<copy_assign_test, copy_assign_test&&>);
    static_assert(std::is_assignable_v<copy_assign_test, const copy_assign_test&>);
    static_assert(std::is_copy_assignable_v<copy_assign_test>);
    static_assert(!std::is_move_assignable_v<copy_assign_test>);

    static_assert(std::is_assignable_v<move_assign_test, move_assign_test>);
    static_assert(!std::is_assignable_v<move_assign_test, move_assign_test&>);
    static_assert(std::is_assignable_v<move_assign_test, move_assign_test&&>);
    static_assert(!std::is_assignable_v<move_assign_test, const move_assign_test&&>);
    static_assert(!std::is_assignable_v<move_assign_test, const move_assign_test&>);
    static_assert(!std::is_copy_assignable_v<move_assign_test>);
    static_assert(std::is_move_assignable_v<move_assign_test>);

    static_assert(!std::is_assignable_v<no_assign_test, no_assign_test>);
    static_assert(!std::is_assignable_v<no_assign_test, no_assign_test&>);
    static_assert(!std::is_assignable_v<no_assign_test, const no_assign_test&>);
    static_assert(!std::is_assignable_v<no_assign_test, no_assign_test&&>);
    static_assert(!std::is_assignable_v<no_assign_test, const no_assign_test&&>);
    static_assert(!std::is_copy_assignable_v<no_assign_test>);
    static_assert(!std::is_move_assignable_v<no_assign_test>);

    static_assert(std::is_assignable_v<both_assign_test, both_assign_test>);
    static_assert(std::is_assignable_v<both_assign_test, const both_assign_test&>);
    static_assert(std::is_assignable_v<both_assign_test, both_assign_test&>);
    static_assert(std::is_assignable_v<both_assign_test, both_assign_test&&>);
    static_assert(std::is_assignable_v<both_assign_test, const both_assign_test&&>);
    static_assert(std::is_copy_assignable_v<both_assign_test>);
    static_assert(std::is_move_assignable_v<both_assign_test>);

    static_assert(std::is_assignable_v<some_assign_test, some_struct>);
    static_assert(std::is_assignable_v<some_assign_test, some_struct&>);
    static_assert(std::is_assignable_v<some_assign_test, some_struct&&>);
    static_assert(std::is_assignable_v<some_assign_test, const some_struct&>);
    static_assert(std::is_assignable_v<some_assign_test, const some_struct&&>);
    static_assert(std::is_copy_assignable_v<some_assign_test>);
    static_assert(std::is_move_assignable_v<some_assign_test>);

    /*
     * core
     */
    static_assert(!assign<copy_assign_test, copy_assign_test>);
    static_assert(assign<copy_assign_test, copy_assign_test&>);
    static_assert(!assign<copy_assign_test, copy_assign_test&&>);
    static_assert(assign<copy_assign_test, const copy_assign_test&>);
    static_assert(copy_assign<copy_assign_test>);
    static_assert(!move_assign<copy_assign_test>);

    static_assert(assign<move_assign_test, move_assign_test>);
    static_assert(!assign<move_assign_test, move_assign_test&>);
    static_assert(assign<move_assign_test, move_assign_test&&>);
    static_assert(!assign<move_assign_test, const move_assign_test&&>);
    static_assert(!assign<move_assign_test, const move_assign_test&>);
    static_assert(!copy_assign<move_assign_test>);
    static_assert(move_assign<move_assign_test>);

    static_assert(!assign<no_assign_test, no_assign_test>);
    static_assert(!assign<no_assign_test, no_assign_test&>);
    static_assert(!assign<no_assign_test, const no_assign_test&>);
    static_assert(!assign<no_assign_test, no_assign_test&&>);
    static_assert(!assign<no_assign_test, const no_assign_test&&>);
    static_assert(!copy_assign<no_assign_test>);
    static_assert(!move_assign<no_assign_test>);

    static_assert(assign<both_assign_test, both_assign_test>);
    static_assert(assign<both_assign_test, const both_assign_test&>);
    static_assert(assign<both_assign_test, both_assign_test&>);
    static_assert(assign<both_assign_test, both_assign_test&&>);
    static_assert(assign<both_assign_test, const both_assign_test&&>);
    static_assert(copy_assign<both_assign_test>);
    static_assert(move_assign<both_assign_test>);

    static_assert(assign<some_assign_test, some_struct>);
    static_assert(assign<some_assign_test, some_struct&>);
    static_assert(assign<some_assign_test, some_struct&&>);
    static_assert(assign<some_assign_test, const some_struct&>);
    static_assert(assign<some_assign_test, const some_struct&&>);
    static_assert(copy_assign<some_assign_test>);
    static_assert(move_assign<some_assign_test>);
}

#include "core/nostd/concepts/convertible_to.hpp"
using core::convertible_to;

TEST_CASE("convertible_to") {
    struct class1 {};
    struct class2 {
        operator class1() const {
            return {};
        }
    };
    struct class3 {
        explicit operator class1() const {
            return {};
        }
    };
    struct class4 {
        class4(const class1&) {}
    };
    struct class5 {
        explicit class5(const class1&) {}
    };

    static_assert(std::convertible_to<int, float>);
    static_assert(std::convertible_to<class2, class1>);
    static_assert(!std::convertible_to<class1, class2>);
    static_assert(!std::convertible_to<class3, class1>);
    static_assert(std::convertible_to<class1, class4>);
    static_assert(!std::convertible_to<class1, class5>);

    static_assert(convertible_to<int, float>);
    static_assert(convertible_to<class2, class1>);
    static_assert(!convertible_to<class1, class2>);
    static_assert(!convertible_to<class3, class1>);
    static_assert(convertible_to<class1, class4>);
    static_assert(!convertible_to<class1, class5>);
}

#include "core/nostd/concepts/ctor.hpp"
using core::copy_ctor;
using core::ctor;
using core::default_ctor;
using core::move_ctor;

TEST_CASE("ctor") {
    struct class1 {};
    struct class2 {
        class2(const class2&) = default;
    };
    struct class3 {
        class3(class3&&) = default;
    };
    struct class4 {
        class4(int) {}
    };
    struct class5 {
        class5(const class5&) = default;
        class5(class5&&) = delete;
    };

    static_assert(std::is_default_constructible_v<class1>);
    static_assert(std::is_default_constructible_v<int>);
    static_assert(!std::is_default_constructible_v<void>);
    static_assert(!std::is_default_constructible_v<int&>);
    static_assert(!std::is_default_constructible_v<class2>);
    static_assert(!std::is_default_constructible_v<class3>);
    static_assert(!std::is_default_constructible_v<class4>);
    static_assert(std::is_constructible_v<class4, float>);
    static_assert(std::is_copy_constructible_v<class1>);
    static_assert(std::is_copy_constructible_v<class2>);
    static_assert(!std::is_copy_constructible_v<class3>);
    static_assert(std::is_copy_constructible_v<class4>);
    static_assert(std::is_move_constructible_v<class1>);
    static_assert(std::is_move_constructible_v<class2>);
    static_assert(std::is_move_constructible_v<class3>);
    static_assert(std::is_move_constructible_v<class4>);
    static_assert(!std::is_move_constructible_v<class5>);

    static_assert(default_ctor<class1>);
    static_assert(default_ctor<int>);
    static_assert(!default_ctor<void>);
    static_assert(!default_ctor<int&>);
    static_assert(!default_ctor<class2>);
    static_assert(!default_ctor<class3>);
    static_assert(!default_ctor<class4>);
    static_assert(ctor<class4, float>);
    static_assert(copy_ctor<class1>);
    static_assert(copy_ctor<class2>);
    static_assert(!copy_ctor<class3>);
    static_assert(copy_ctor<class4>);
    static_assert(move_ctor<class1>);
    static_assert(move_ctor<class2>);
    static_assert(move_ctor<class3>);
    static_assert(move_ctor<class4>);
    static_assert(!move_ctor<class5>);
}

#include "core/nostd/concepts/inheritance.hpp"
using core::empty_class;
using core::final_class;
using core::base_of;

TEST_CASE("inheritance") {
    struct empty {};
    struct not_empty {
        char _;
    };
    struct final1 final : empty {};
    struct not_final : empty {};

    static_assert(std::is_empty_v<empty>);
    static_assert(!std::is_empty_v<int>);
    static_assert(!std::is_empty_v<not_empty>);
    static_assert(std::is_final_v<final1>);
    static_assert(!std::is_final_v<not_final>);
    static_assert(std::is_base_of_v<empty, final1>);
    static_assert(std::is_base_of_v<empty, empty>);
    static_assert(!std::is_base_of_v<empty, not_empty>);

    static_assert(empty_class<empty>);
    static_assert(!empty_class<int>);
    static_assert(!empty_class<not_empty>);
    static_assert(final_class<final1>);
    static_assert(!final_class<not_final>);
    static_assert(base_of<empty, final1>);
    static_assert(base_of<empty, empty>);
    static_assert(!base_of<empty, not_empty>);
}

#include "core/nostd/concepts/invocable.hpp"
using core::invocable;
using core::invocable_r;

TEST_CASE("invocable") {
    auto f1 = [](int, float) { return 1; };
    auto f2 = [](auto v) { if constexpr (std::is_same_v<decltype(v), int>) return ""; else return 0;};

    static_assert(std::is_invocable_v<decltype(f1), int, int>);
    static_assert(std::is_invocable_r_v<float, decltype(f1), int, int>);
    static_assert(std::is_invocable_r_v<int, decltype(f2), float>);
    static_assert(!std::is_invocable_r_v<int, decltype(f2), int>);
    static_assert(std::is_invocable_r_v<const char*, decltype(f2), int>);

    static_assert(invocable<decltype(f1), int, int>);
    static_assert(invocable_r<float, decltype(f1), int, int>);
    static_assert(invocable_r<int, decltype(f2), float>);
    static_assert(!invocable_r<int, decltype(f2), int>);
    static_assert(invocable_r<const char*, decltype(f2), int>);
}

#include "core/nostd/concepts/nothrow_assign.hpp"
using core::nothrow_assign;
using core::nothrow_copy_assign;
using core::nothrow_move_assign;

TEST_CASE("nothrow_assign") {
    struct test1 {
        test1& operator=(const test1&) noexcept {
            return *this;
        }
        test1& operator=(test1&&) {
            return *this;
        }
    };

    struct test2 {
        test2& operator=(const test2&) {
            return *this;
        }
        test2& operator=(test2&&) noexcept {
            return *this;
        }
    };

    struct test3 {
        test3& operator=(int) {
            return *this;
        }
    };

    struct test4 {
        test4& operator=(int) noexcept {
            return *this;
        }
    };

    static_assert(std::is_nothrow_assignable_v<test4, int>);
    static_assert(!std::is_nothrow_assignable_v<test3, int>);
    static_assert(!std::is_nothrow_assignable_v<test4, const char*>);

    static_assert(std::is_nothrow_copy_assignable_v<test1>);
    static_assert(!std::is_nothrow_copy_assignable_v<test2>);

    static_assert(!std::is_nothrow_move_assignable_v<test1>);
    static_assert(std::is_nothrow_move_assignable_v<test2>);

    static_assert(nothrow_assign<test4, int>);
    static_assert(!nothrow_assign<test3, int>);
    static_assert(!nothrow_assign<test4, const char*>);

    static_assert(nothrow_copy_assign<test1>);
    static_assert(!nothrow_copy_assign<test2>);

    static_assert(!nothrow_move_assign<test1>);
    static_assert(nothrow_move_assign<test2>);
}

#include "core/nostd/concepts/nothrow_ctor.hpp"
using core::nothrow_ctor;
using core::nothrow_copy_ctor;
using core::nothrow_move_ctor;

TEST_CASE("nothrow_ctor") {
    struct test1 {
        test1(const test1&) noexcept {}
        test1(test1&&) {}
    };

    struct test2 {
        test2(const test2&) {}
        test2(test2&&) noexcept {}
    };

    struct test3 {
        test3(int) {}
    };

    struct test4 {
        test4(int) noexcept {}
    };

    static_assert(std::is_nothrow_constructible_v<test4, int>);
    static_assert(!std::is_nothrow_constructible_v<test3, int>);
    static_assert(!std::is_nothrow_constructible_v<test4, const char*>);

    static_assert(std::is_nothrow_copy_constructible_v<test1>);
    static_assert(!std::is_nothrow_copy_constructible_v<test2>);

    static_assert(!std::is_nothrow_move_constructible_v<test1>);
    static_assert(std::is_nothrow_move_constructible_v<test2>);

    static_assert(nothrow_ctor<test4, int>);
    static_assert(!nothrow_ctor<test3, int>);
    static_assert(!nothrow_ctor<test4, const char*>);

    static_assert(nothrow_copy_ctor<test1>);
    static_assert(!nothrow_copy_ctor<test2>);

    static_assert(!nothrow_move_ctor<test1>);
    static_assert(nothrow_move_ctor<test2>);
}

#include "core/nostd/concepts/referenceable.hpp"
using core::referenceable;

TEST_CASE("referenceable") {
    struct class1 {};
    static_assert(referenceable<int>);
    static_assert(referenceable<int()>);
    static_assert(referenceable<int&>);
    static_assert(!referenceable<int()const>);
    static_assert(referenceable<void(class1::*)()>);
    static_assert(!referenceable<void>);
    static_assert(referenceable<int[]>);
}

#include "core/nostd/concepts/same_as.hpp"
using core::same_as;

TEST_CASE("same_as") {
    static_assert(same_as<int, int>);
    static_assert(!same_as<int, int&&>);
    static_assert(!same_as<int, float>);
}

#include "core/nostd/concepts/trivial_assign.hpp"
using core::trivial_assign;
using core::trivial_copy_assign;
using core::trivial_move_assign;

TEST_CASE("trivial_assign") {
    struct test1 {};
    struct test2 {
        test2& operator=(const test2&) = default;
        test2& operator=(test2&&) noexcept {
            return *this;
        }
    };
    struct test3 {
        test3& operator=(const test3&) {
            return *this;
        }
        test3& operator=(test3&&) noexcept {
            return *this;
        }
    };

    struct test4 {};
    struct test5 {
        operator test4() const {
            return {};
        }
    };

    static_assert(std::is_trivially_copy_assignable_v<test1>);
    static_assert(std::is_trivially_copy_assignable_v<test2>);
    static_assert(!std::is_trivially_copy_assignable_v<test3>);
    static_assert(std::is_trivially_move_assignable_v<test1>);
    static_assert(!std::is_trivially_move_assignable_v<test2>);
    static_assert(std::is_trivially_move_assignable_v<test1>);
    static_assert(std::is_trivially_copy_assignable_v<int>);
    static_assert(std::is_trivially_assignable_v<int&, int>);
    static_assert(!std::is_trivially_assignable_v<int, int>);
    static_assert(std::is_trivially_assignable_v<int&, const int&&>);
    static_assert(std::is_trivially_assignable_v<int&, const float&&>);
    static_assert(!std::is_trivially_assignable_v<test4, const test5&>);

    static_assert(trivial_copy_assign<test1>);
    static_assert(trivial_copy_assign<test2>);
    static_assert(!trivial_copy_assign<test3>);
    static_assert(trivial_move_assign<test1>);
    static_assert(!trivial_move_assign<test2>);
    static_assert(trivial_move_assign<test1>);
    static_assert(trivial_copy_assign<int>);
    static_assert(trivial_assign<int&, int>);
    static_assert(!trivial_assign<int, int>);
    static_assert(trivial_assign<int&, const int&&>);
    static_assert(trivial_assign<int&, const float&&>);
    static_assert(!trivial_assign<test4&, const test5&>);
}

#include "core/nostd/concepts/trivial_ctor.hpp"
using core::trivial_ctor;
using core::trivial_copy_ctor;
using core::trivial_move_ctor;

TEST_CASE("trivial_ctor") {
    struct test1 {};
    struct test2 {
        test2(const test2&) = default;
        test2(test2&&) noexcept {}
    };
    struct test3 {
        test3(const test3&) {}
        test3(test3&&) = default;
    };

    struct test4 {};
    struct test5 {
        operator test4() const {
            return {};
        }
    };

    struct test6 {
        int a;
    };

    static_assert(std::is_trivially_copy_constructible_v<test1>);
    static_assert(std::is_trivially_copy_constructible_v<test2>);
    static_assert(!std::is_trivially_copy_constructible_v<test3>);
    static_assert(std::is_trivially_move_constructible_v<test1>);
    static_assert(!std::is_trivially_move_constructible_v<test2>);
    static_assert(std::is_trivially_move_constructible_v<test1>);
    static_assert(std::is_trivially_copy_constructible_v<int>);
    static_assert(std::is_trivially_constructible_v<int, int>);
    static_assert(std::is_trivially_constructible_v<int, const float&&>);
    static_assert(!std::is_trivially_constructible_v<test4, const test5&>);
    static_assert(std::is_trivially_constructible_v<test6, int>);

    static_assert(trivial_copy_ctor<test1>);
    static_assert(trivial_copy_ctor<test2>);
    static_assert(!trivial_copy_ctor<test3>);
    static_assert(trivial_move_ctor<test1>);
    static_assert(!trivial_move_ctor<test2>);
    static_assert(trivial_move_ctor<test1>);
    static_assert(trivial_copy_ctor<int>);
    static_assert(trivial_ctor<int, int>);
    static_assert(trivial_ctor<int, const float&&>);
    static_assert(!trivial_ctor<test4, const test5&>);
    static_assert(trivial_ctor<test6, int>);
}

#include "core/nostd/concepts/trivial_dtor.hpp"
using core::trivial_dtor;

TEST_CASE("trivial_dtor") {
    struct test1 {};
    struct test2 {
        ~test2() = default;
    };
    struct test3 {
        ~test3() {}
    };

    static_assert(std::is_trivially_destructible_v<int&>);
    static_assert(std::is_trivially_destructible_v<test1>);
    static_assert(std::is_trivially_destructible_v<test2>);
    static_assert(!std::is_trivially_destructible_v<test3>);

    static_assert(trivial_dtor<int&>);
    static_assert(trivial_dtor<test1>);
    static_assert(trivial_dtor<test2>);
    static_assert(!trivial_dtor<test3>);
}

#include "core/nostd/concepts/variant.hpp"
#include <variant>
using core::variant;

TEST_CASE("variant concept") {
    static_assert(variant<std::variant<int, float>>);
}
