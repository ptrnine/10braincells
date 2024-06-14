#include <type_traits>
#include <catch2/catch_test_macros.hpp>

#include "core/traits/is_same.hpp"
#define ENSURE_SAME(...) static_assert(std::is_same_v<__VA_ARGS__>)

#include "core/traits/add_ptr.hpp"
using core::add_ptr;

TEST_CASE("add_pointer") {
    ENSURE_SAME(std::add_pointer_t<void>, void*);
    ENSURE_SAME(add_ptr<void>, void*);

    ENSURE_SAME(std::add_pointer_t<void()>, void(*)());
    ENSURE_SAME(std::add_pointer_t<void() const>, void() const);
    ENSURE_SAME(std::add_pointer_t<void() noexcept>, void(*)() noexcept);
    ENSURE_SAME(std::add_pointer_t<void() volatile>, void() volatile);
    ENSURE_SAME(std::add_pointer_t<void() const volatile>, void() const volatile);
    ENSURE_SAME(std::add_pointer_t<void() &>, void() &);
    ENSURE_SAME(std::add_pointer_t<void() &&>, void() &&);

    ENSURE_SAME(add_ptr<void()>, void(*)());
    ENSURE_SAME(add_ptr<void() const>, void() const);
    ENSURE_SAME(add_ptr<void() noexcept>, void(*)() noexcept);
    ENSURE_SAME(add_ptr<void() volatile>, void() volatile);
    ENSURE_SAME(add_ptr<void() const volatile>, void() const volatile);
    ENSURE_SAME(add_ptr<void() &>, void() &);
    ENSURE_SAME(add_ptr<void() &&>, void() &&);

    struct Class {};
    ENSURE_SAME(std::add_pointer_t<void(Class::*)() const>, void(Class::**)() const);
    ENSURE_SAME(add_ptr<void(Class::*)() const>, void(Class::**)() const);
}

#include "core/traits/add_ref.hpp"
using core::add_lvalue_ref;
using core::add_rvalue_ref;

TEST_CASE("add_ref") {
    ENSURE_SAME(std::add_lvalue_reference_t<void>, void);
    ENSURE_SAME(std::add_rvalue_reference_t<void>, void);
    ENSURE_SAME(add_lvalue_ref<void>, void);
    ENSURE_SAME(add_rvalue_ref<void>, void);

    struct Class {};

    ENSURE_SAME(std::add_lvalue_reference_t<void()>, void(&)());
    ENSURE_SAME(std::add_lvalue_reference_t<void() const>, void() const);
    ENSURE_SAME(std::add_lvalue_reference_t<void() noexcept>, void(&)() noexcept);
    ENSURE_SAME(std::add_lvalue_reference_t<void() volatile>, void() volatile);
    ENSURE_SAME(std::add_lvalue_reference_t<void() const volatile>, void() const volatile);
    ENSURE_SAME(std::add_lvalue_reference_t<void() &>, void() &);
    ENSURE_SAME(std::add_lvalue_reference_t<void() &&>, void() &&);
    ENSURE_SAME(std::add_lvalue_reference_t<void(Class::*)() const>, void(Class::*&)() const);

    ENSURE_SAME(std::add_rvalue_reference_t<void()>, void(&&)());
    ENSURE_SAME(std::add_rvalue_reference_t<void() const>, void() const);
    ENSURE_SAME(std::add_rvalue_reference_t<void() noexcept>, void(&&)() noexcept);
    ENSURE_SAME(std::add_rvalue_reference_t<void() volatile>, void() volatile);
    ENSURE_SAME(std::add_rvalue_reference_t<void() const volatile>, void() const volatile);
    ENSURE_SAME(std::add_rvalue_reference_t<void() &>, void() &);
    ENSURE_SAME(std::add_rvalue_reference_t<void() &&>, void() &&);
    ENSURE_SAME(std::add_rvalue_reference_t<void(Class::*)() const>, void(Class::*&&)() const);

    ENSURE_SAME(add_lvalue_ref<void()>, void(&)());
    ENSURE_SAME(add_lvalue_ref<void() const>, void() const);
    ENSURE_SAME(add_lvalue_ref<void() noexcept>, void(&)() noexcept);
    ENSURE_SAME(add_lvalue_ref<void() volatile>, void() volatile);
    ENSURE_SAME(add_lvalue_ref<void() const volatile>, void() const volatile);
    ENSURE_SAME(add_lvalue_ref<void() &>, void() &);
    ENSURE_SAME(add_lvalue_ref<void() &&>, void() &&);
    ENSURE_SAME(add_lvalue_ref<void(Class::*)() const>, void(Class::*&)() const);

    ENSURE_SAME(add_rvalue_ref<void()>, void(&&)());
    ENSURE_SAME(add_rvalue_ref<void() const>, void() const);
    ENSURE_SAME(add_rvalue_ref<void() noexcept>, void(&&)() noexcept);
    ENSURE_SAME(add_rvalue_ref<void() volatile>, void() volatile);
    ENSURE_SAME(add_rvalue_ref<void() const volatile>, void() const volatile);
    ENSURE_SAME(add_rvalue_ref<void() &>, void() &);
    ENSURE_SAME(add_rvalue_ref<void() &&>, void() &&);
    ENSURE_SAME(add_rvalue_ref<void(Class::*)() const>, void(Class::*&&)() const);
}

#include "core/traits/conditional.hpp"
using core::conditional;

TEST_CASE("conditional") {
    struct class1 {};
    struct class2 {};

    ENSURE_SAME(conditional<false, class1, class2>, class2);
    ENSURE_SAME(conditional<true, class1, class2>, class1);
}

#include "core/traits/decay.hpp"
using core::decay;

TEST_CASE("decay") {
    ENSURE_SAME(std::decay_t<void*>, void*);
    ENSURE_SAME(std::decay_t<int(&)[3]>, int*);
    ENSURE_SAME(std::decay_t<int()>, int(*)());
    ENSURE_SAME(std::decay_t<const int&>, int);
    ENSURE_SAME(std::decay_t<int&&>, int);
    ENSURE_SAME(std::decay_t<void>, void);
    ENSURE_SAME(std::decay_t<int(&)[3][4]>, int(*)[4]);

    ENSURE_SAME(decay<void*>, void*);
    ENSURE_SAME(decay<int(&)[3]>, int*);
    ENSURE_SAME(decay<int()>, int(*)());
    ENSURE_SAME(decay<const int&>, int);
    ENSURE_SAME(decay<int&&>, int);
    ENSURE_SAME(decay<void>, void);
    ENSURE_SAME(decay<int(&)[3][4]>, int(*)[4]);
}

#include "core/traits/declval.hpp"
using core::declval;

template <typename T>
concept declval_ok = requires {
    {declval<T>()};
};

template <typename T>
concept std_declval_ok = requires {
    {std::declval<T>()};
};

TEST_CASE("declval") {
    struct class1 {};

    ENSURE_SAME(decltype(std::declval<int[3]>()), int(&&)[3]);
    ENSURE_SAME(decltype(std::declval<void>()), void);
    ENSURE_SAME(decltype(std::declval<void()>()), void(&)());
    ENSURE_SAME(decltype(std::declval<void(class1::*)()>()), void(class1::*&&)());
    static_assert(std_declval_ok<int>);
    static_assert(!std_declval_ok<void() const>);

    ENSURE_SAME(decltype(declval<int[3]>()), int(&&)[3]);
    ENSURE_SAME(decltype(declval<void>()), void);
    ENSURE_SAME(decltype(declval<void()>()), void(&)());
    ENSURE_SAME(decltype(declval<void(class1::*)()>()), void(class1::*&&)());
    static_assert(declval_ok<int>);
    static_assert(!declval_ok<void() const>);
}

#include "core/traits/idx_of_type.hpp"
using core::idx_of_type;
using core::idx_of_type_pack;

template <typename... Ts>
struct test1 {};

TEST_CASE("idx_of_type") {
    static_assert(idx_of_type<void, int, void, float> == 1);
    static_assert(idx_of_type<int, int, void, float> == 0);
    static_assert(idx_of_type<float, int, void, float> == 2);

    static_assert(idx_of_type_pack<void, test1<int, void, float>> == 1);
    static_assert(idx_of_type_pack<int, test1<int, void, float>> == 0);
    static_assert(idx_of_type_pack<float, test1<int, void, float>> == 2);
}

#include "core/traits/is_array.hpp"
using core::is_array;
using core::is_bounded_array;

TEST_CASE("is_array") {
    static_assert(std::is_array_v<int[]>);
    static_assert(std::is_array_v<int[2]>);
    static_assert(std::is_array_v<int[3][3]>);
    static_assert(!std::is_array_v<int(&)[2]>);
    static_assert(!std::is_array_v<int(*)[3]>);
    static_assert(std::is_bounded_array_v<int[3]>);
    static_assert(!std::is_bounded_array_v<int[]>);

    static_assert(is_array<int[]>);
    static_assert(is_array<int[2]>);
    static_assert(is_array<int[3][3]>);
    static_assert(!is_array<int(&)[2]>);
    static_assert(!is_array<int(*)[3]>);
    static_assert(is_bounded_array<int[3]>);
    static_assert(!is_bounded_array<int[]>);
}

#include "core/traits/is_const.hpp"
using core::is_const;

TEST_CASE("is_const") {
    static_assert(std::is_const_v<const int>);
    static_assert(std::is_const_v<int** const>);
    static_assert(!std::is_const_v<const int* const*>);
    static_assert(!std::is_const_v<std::add_const_t<void()>>);

    static_assert(is_const<const int>);
    static_assert(is_const<int** const>);
    static_assert(!is_const<const int* const*>);
    static_assert(!is_const<std::add_const_t<void()>>);
}

#include "core/traits/is_destructible.hpp"
using core::is_destructible;

TEST_CASE("is_destructible") {
    struct class1 {};
    struct class2 { ~class2() = delete; };

    static_assert(std::is_destructible_v<int>);
    static_assert(std::is_destructible_v<class1>);
    static_assert(std::is_destructible_v<int(*)()>);
    static_assert(std::is_destructible_v<int(&)()>);
    static_assert(std::is_destructible_v<class2&>);
    static_assert(!std::is_destructible_v<class2>);
    static_assert(!std::is_destructible_v<int[]>);
    static_assert(!std::is_destructible_v<int()>);
    static_assert(!std::is_destructible_v<void>);

    static_assert(is_destructible<int>);
    static_assert(is_destructible<class1>);
    static_assert(is_destructible<int(*)()>);
    static_assert(is_destructible<int(&)()>);
    static_assert(is_destructible<class2&>);
    static_assert(!is_destructible<class2>);
    static_assert(!is_destructible<int[]>);
    static_assert(!is_destructible<int()>);
    static_assert(!is_destructible<void>);
}

#include "core/traits/is_function.hpp"
using core::is_function;

TEST_CASE("is_function") {
    struct class1 {};

    static_assert(std::is_function_v<void()>);
    static_assert(std::is_function_v<void() const>);
    static_assert(std::is_function_v<void() const volatile>);
    static_assert(!std::is_function_v<void(class1::*)()>);
    static_assert(!std::is_function_v<void(*)()>);
    static_assert(!std::is_function_v<void(&)()>);
    static_assert(!std::is_function_v<void(&&)()>);
    static_assert(!std::is_function_v<void>);
    static_assert(!std::is_function_v<int>);

    static_assert(is_function<void()>);
    static_assert(is_function<void() const>);
    static_assert(is_function<void() const volatile>);
    static_assert(!is_function<void(class1::*)()>);
    static_assert(!is_function<void(*)()>);
    static_assert(!is_function<void(&)()>);
    static_assert(!is_function<void(&&)()>);
    static_assert(!is_function<void>);
    static_assert(!is_function<int>);
}

#include "core/traits/is_ptr.hpp"
using core::is_ptr;

TEST_CASE("is_pointer") {
    struct class1 {};

    static_assert(std::is_pointer_v<int*>);
    static_assert(std::is_pointer_v<int(*)()>);
    static_assert(std::is_pointer_v<int(*)[3]>);
    static_assert(std::is_pointer_v<const volatile int* const volatile>);
    static_assert(!std::is_pointer_v<int*&>);
    static_assert(!std::is_pointer_v<int()>);
    static_assert(!std::is_pointer_v<int(class1::*)()>);
    static_assert(!std::is_pointer_v<int[]>);

    static_assert(is_ptr<int*>);
    static_assert(is_ptr<int(*)()>);
    static_assert(is_ptr<int(*)[3]>);
    static_assert(is_ptr<const volatile int* const volatile>);
    static_assert(!is_ptr<int*&>);
    static_assert(!is_ptr<int()>);
    static_assert(!is_ptr<int(class1::*)()>);
    static_assert(!is_ptr<int[]>);
}

#include "core/traits/is_ref.hpp"
using core::is_ref;

TEST_CASE("is_ref") {
    static_assert(std::is_reference_v<int&>);
    static_assert(std::is_reference_v<int&&>);
    static_assert(std::is_reference_v<const int&>);
    static_assert(std::is_reference_v<void*&>);
    static_assert(std::is_reference_v<void(&)()>);
    static_assert(std::is_reference_v<int(&&)[3]>);

    static_assert(is_ref<int&>);
    static_assert(is_ref<int&&>);
    static_assert(is_ref<const int&>);
    static_assert(is_ref<void*&>);
    static_assert(is_ref<void(&)()>);
    static_assert(is_ref<int(&&)[3]>);
}

#include "core/traits/is_same.hpp"
using core::is_same;

TEST_CASE("is_same") {
    static_assert(std::is_same_v<const int&(&&)(), const int&(&&)()>);
    static_assert(is_same<const int&(&&)(), const int&(&&)()>);
}

#include "core/traits/remove_const.hpp"
using core::remove_const;

TEST_CASE("remove_const") {
    ENSURE_SAME(std::remove_const_t<void* const>, void*);
    ENSURE_SAME(std::remove_const_t<const int(&)[3]>, const int(&)[3]);
    ENSURE_SAME(std::remove_const_t<const int[3]>, int[3]);
    ENSURE_SAME(std::remove_const_t<const int&&>, const int&&);
    ENSURE_SAME(std::remove_const_t<const int>, int);

    ENSURE_SAME(remove_const<void* const>, void*);
    ENSURE_SAME(remove_const<const int(&)[3]>, const int(&)[3]);
    ENSURE_SAME(remove_const<const int[3]>, int[3]);
    ENSURE_SAME(remove_const<const int&&>, const int&&);
    ENSURE_SAME(remove_const<const int>, int);
}

#include "core/traits/remove_cv.hpp"
using core::remove_cv;

TEST_CASE("remove_cv") {
    ENSURE_SAME(std::remove_cv_t<void* const volatile>, void*);
    ENSURE_SAME(std::remove_cv_t<const volatile int(&)[3]>, const volatile int(&)[3]);
    ENSURE_SAME(std::remove_cv_t<const volatile int[3]>, int[3]);
    ENSURE_SAME(std::remove_cv_t<const volatile int&&>, const volatile int&&);
    ENSURE_SAME(std::remove_cv_t<const volatile int>, int);

    ENSURE_SAME(remove_cv<void* const volatile>, void*);
    ENSURE_SAME(remove_cv<const volatile int(&)[3]>, const volatile int(&)[3]);
    ENSURE_SAME(remove_cv<const volatile int[3]>, int[3]);
    ENSURE_SAME(remove_cv<const volatile int&&>, const volatile int&&);
    ENSURE_SAME(remove_cv<const volatile int>, int);
}

#include "core/traits/remove_extent.hpp"
using core::remove_extent;
using core::remove_all_extents;
using core::remove_sized_extent;
using core::remove_all_sized_extents;

TEST_CASE("remove_extent") {
    ENSURE_SAME(std::remove_extent_t<int[]>, int);
    ENSURE_SAME(std::remove_extent_t<int[2]>, int);
    ENSURE_SAME(std::remove_extent_t<int[2][2]>, int[2]);
    ENSURE_SAME(std::remove_extent_t<int[][7]>, int[7]);
    ENSURE_SAME(std::remove_extent_t<int(&)[2]>, int(&)[2]);
    ENSURE_SAME(std::remove_extent_t<int(*)[2]>, int(*)[2]);
    ENSURE_SAME(std::remove_all_extents_t<int[1][1][1]>, int);
    ENSURE_SAME(std::remove_all_extents_t<int(&)[1][1][1]>, int(&)[1][1][1]);

    ENSURE_SAME(remove_extent<int[]>, int);
    ENSURE_SAME(remove_extent<int[2]>, int);
    ENSURE_SAME(remove_extent<int[2][2]>, int[2]);
    ENSURE_SAME(remove_extent<int[][7]>, int[7]);
    ENSURE_SAME(remove_extent<int(&)[2]>, int(&)[2]);
    ENSURE_SAME(remove_extent<int(*)[2]>, int(*)[2]);
    ENSURE_SAME(remove_all_extents<int[1][1][1]>, int);
    ENSURE_SAME(remove_all_extents<int(&)[1][1][1]>, int(&)[1][1][1]);

    ENSURE_SAME(remove_sized_extent<int(&)[]>, int(&)[]);
    ENSURE_SAME(remove_sized_extent<int[]>, int[]);
    ENSURE_SAME(remove_sized_extent<int[7]>, int);
    ENSURE_SAME(remove_sized_extent<int[][7]>, int[][7]);
    ENSURE_SAME(remove_sized_extent<int[1][1]>, int[1]);

    ENSURE_SAME(remove_all_sized_extents<int[][3]>, int[][3]);
    ENSURE_SAME(remove_all_sized_extents<int[3][3]>, int);
    ENSURE_SAME(remove_all_sized_extents<int[]>, int[]);
}

#include "core/traits/remove_ref.hpp"
using core::remove_ref;
using core::remove_const_ref;

TEST_CASE("remove_ref") {
    struct class1 {};

    ENSURE_SAME(std::remove_reference_t<int>, int);
    ENSURE_SAME(std::remove_reference_t<int&>, int);
    ENSURE_SAME(std::remove_reference_t<int&&>, int);
    ENSURE_SAME(std::remove_reference_t<const int&>, const int);
    ENSURE_SAME(std::remove_reference_t<void(&)()>, void());
    ENSURE_SAME(std::remove_reference_t<void(class1::*)()>, void(class1::*)());
    ENSURE_SAME(std::remove_reference_t<int(&&)[3]>, int[3]);
    ENSURE_SAME(std::remove_reference_t<int const* &>, const int*);

    ENSURE_SAME(remove_ref<int>, int);
    ENSURE_SAME(remove_ref<int&>, int);
    ENSURE_SAME(remove_ref<int&&>, int);
    ENSURE_SAME(remove_ref<const int&>, const int);
    ENSURE_SAME(remove_ref<void(&)()>, void());
    ENSURE_SAME(remove_ref<void(class1::*)()>, void(class1::*)());
    ENSURE_SAME(remove_ref<int(&&)[3]>, int[3]);
    ENSURE_SAME(remove_ref<int const* const&>, const int* const);

    ENSURE_SAME(remove_const_ref<const int&&>, int);
    ENSURE_SAME(remove_const_ref<int const* const&>, const int*);
    ENSURE_SAME(std::remove_const_t<std::remove_reference_t<int const* const&>>, const int*);
}

#include <core/traits/remove_cvref.hpp>
using core::remove_cvref;

TEST_CASE("remove_cvref") {
    struct class1 {};

    ENSURE_SAME(std::remove_cvref_t<const volatile int>, int);
    ENSURE_SAME(std::remove_cvref_t<const volatile int&>, int);
    ENSURE_SAME(std::remove_cvref_t<const volatile int&&>, int);
    ENSURE_SAME(std::remove_cvref_t<const int&>, int);
    ENSURE_SAME(std::remove_cvref_t<void(&)()>, void());
    ENSURE_SAME(std::remove_cvref_t<void(class1::*)()const>, void(class1::*)()const);
    ENSURE_SAME(std::remove_cvref_t<const volatile int(&&)[3]>, int[3]);
    ENSURE_SAME(std::remove_cvref_t<int volatile const* volatile&>, const volatile int*);

    ENSURE_SAME(remove_cvref<const volatile int>, int);
    ENSURE_SAME(remove_cvref<const volatile int&>, int);
    ENSURE_SAME(remove_cvref<const volatile int&&>, int);
    ENSURE_SAME(remove_cvref<const int&>, int);
    ENSURE_SAME(remove_cvref<void(&)()>, void());
    ENSURE_SAME(remove_cvref<void(class1::*)()const>, void(class1::*)()const);
    ENSURE_SAME(remove_cvref<const volatile int(&&)[3]>, int[3]);
    ENSURE_SAME(remove_cvref<int volatile const* volatile&>, const volatile int*);
}

#include "core/traits/type_at_idx.hpp"
using core::type_at_idx;
using core::type_at_idx_pack;

template <typename...>
struct test2;

TEST_CASE("type_at_idx") {
    ENSURE_SAME(type_at_idx_pack<0, test2<int, float, void, void>>, int);
    ENSURE_SAME(type_at_idx_pack<1, test2<int, float, void, void>>, float);
    ENSURE_SAME(type_at_idx_pack<2, test2<int, float, void, void>>, void);
    ENSURE_SAME(type_at_idx_pack<3, test2<int, float, void, void>>, void);

    ENSURE_SAME(type_at_idx<0, int, float, void, void>, int);
    ENSURE_SAME(type_at_idx<1, int, float, void, void>, float);
    ENSURE_SAME(type_at_idx<2, int, float, void, void>, void);
    ENSURE_SAME(type_at_idx<3, int, float, void, void>, void);
}

#include "core/traits/make_signed.hpp"
using core::make_signed;

TEST_CASE("make_signed") {
    ENSURE_SAME(make_signed<core::u8>, core::i8);
    ENSURE_SAME(make_signed<core::u16>, core::i16);
    ENSURE_SAME(make_signed<core::u32>, core::i32);
    ENSURE_SAME(make_signed<core::u64>, core::i64);
}

#include "core/traits/add_const.hpp"
#include "core/traits/add_volatile.hpp"
#include "core/traits/add_cv.hpp"
using core::add_const;
using core::add_volatile;
using core::add_cv;

TEST_CASE("add_const/add_volatile/add_cv") {
    ENSURE_SAME(add_const<int>, const int);
    ENSURE_SAME(std::add_const_t<int>, const int);

    ENSURE_SAME(std::add_const_t<int&>, int&);
    ENSURE_SAME(add_const<int&>, int&);

    ENSURE_SAME(std::add_const_t<int()>, int());
    ENSURE_SAME(add_const<int()>, int());

    ENSURE_SAME(add_volatile<int>, volatile int);
    ENSURE_SAME(std::add_volatile_t<int>, volatile int);

    ENSURE_SAME(std::add_volatile_t<int&>, int&);
    ENSURE_SAME(add_volatile<int&>, int&);

    ENSURE_SAME(std::add_volatile_t<int()>, int());
    ENSURE_SAME(add_volatile<int()>, int());

    ENSURE_SAME(add_cv<int>, const volatile int);
    ENSURE_SAME(std::add_cv_t<int>, const volatile int);

    ENSURE_SAME(std::add_cv_t<int&>, int&);
    ENSURE_SAME(add_cv<int&>, int&);

    ENSURE_SAME(std::add_cv_t<int()>, int());
    ENSURE_SAME(add_cv<int()>, int());
}

#include "core/traits/invoke.hpp"
using core::invoke_result;

struct func2 {
    auto operator()(auto v) const noexcept {
        return v;
    }
};

TEST_CASE("invoke_result") {
    ENSURE_SAME(invoke_result<int(*)()>, int);
    ENSURE_SAME(std::invoke_result_t<int(*)()>, int);
    ENSURE_SAME(invoke_result<int(*)(int), const int&>, int);
    ENSURE_SAME(std::invoke_result_t<int(*)(int), const int&>, int);
    ENSURE_SAME(invoke_result<int&&(*)(int), const int&>, int&&);
    ENSURE_SAME(std::invoke_result_t<int&&(*)(int), const int&>, int&&);
    ENSURE_SAME(invoke_result<void()>, void);
    ENSURE_SAME(std::invoke_result_t<void()>, void);

    struct func {
        float operator()(int) {
            return 3;
        }
        float operator()(func) const {
            return 3;
        }
    };
    ENSURE_SAME(invoke_result<func, int&&>, float);
    ENSURE_SAME(std::invoke_result_t<func, int&&>, float);
    ENSURE_SAME(invoke_result<func, func&&>, float);
    ENSURE_SAME(std::invoke_result_t<func, func&&>, float);

    ENSURE_SAME(invoke_result<func2, const char*>, const char*);
    ENSURE_SAME(std::invoke_result_t<func2, const char*>, const char*);
    ENSURE_SAME(invoke_result<func2, const int&>, int);
    ENSURE_SAME(std::invoke_result_t<func2, const int&>, int);

    struct class1 {
        int f1(float) &;
        int f2(float) const&;
        int f3(float) &&;
        int f4(float) const&&;
        int f5(float);
        int f6(float) const;
        int f7(float) & noexcept;
        int f8(float) const& noexcept;
        int f9(float) && noexcept;
        int f10(float) const&& noexcept;
        int f11(float) noexcept;
        int f12(float) const noexcept;
    };

    ENSURE_SAME(invoke_result<decltype(&class1::f1), class1&, float>, int);
    ENSURE_SAME(std::invoke_result_t<decltype(&class1::f1), class1&, float>, int);

    ENSURE_SAME(invoke_result<decltype(&class1::f2), const class1&, float>, int);
    ENSURE_SAME(invoke_result<decltype(&class1::f2), class1&, float>, int);
    ENSURE_SAME(invoke_result<decltype(&class1::f2), class1, float>, int);
    ENSURE_SAME(invoke_result<decltype(&class1::f2), const class1, float>, int);
    ENSURE_SAME(invoke_result<decltype(&class1::f2), class1&&, float>, int);
    ENSURE_SAME(invoke_result<decltype(&class1::f2), const class1&&, float>, int);
    ENSURE_SAME(std::invoke_result_t<decltype(&class1::f2), const class1&, float>, int);
    ENSURE_SAME(std::invoke_result_t<decltype(&class1::f2), class1&, float>, int);
    ENSURE_SAME(std::invoke_result_t<decltype(&class1::f2), class1, float>, int);
    ENSURE_SAME(std::invoke_result_t<decltype(&class1::f2), const class1, float>, int);
    ENSURE_SAME(std::invoke_result_t<decltype(&class1::f2), class1&&, float>, int);
    ENSURE_SAME(std::invoke_result_t<decltype(&class1::f2), const class1&&, float>, int);

    ENSURE_SAME(invoke_result<decltype(&class1::f3), class1&&, float>, int);
    ENSURE_SAME(invoke_result<decltype(&class1::f3), class1, float>, int);
    ENSURE_SAME(std::invoke_result_t<decltype(&class1::f3), class1&&, float>, int);
    ENSURE_SAME(std::invoke_result_t<decltype(&class1::f3), class1, float>, int);

    ENSURE_SAME(invoke_result<decltype(&class1::f4), const class1&&, float>, int);
    ENSURE_SAME(invoke_result<decltype(&class1::f4), class1&&, float>, int);
    ENSURE_SAME(invoke_result<decltype(&class1::f4), class1, float>, int);
    ENSURE_SAME(invoke_result<decltype(&class1::f4), const class1, float>, int);
    ENSURE_SAME(std::invoke_result_t<decltype(&class1::f4), const class1&&, float>, int);
    ENSURE_SAME(std::invoke_result_t<decltype(&class1::f4), class1&&, float>, int);
    ENSURE_SAME(std::invoke_result_t<decltype(&class1::f4), class1, float>, int);
    ENSURE_SAME(std::invoke_result_t<decltype(&class1::f4), const class1, float>, int);

    ENSURE_SAME(invoke_result<decltype(&class1::f5), class1, float>, int);
    ENSURE_SAME(invoke_result<decltype(&class1::f5), class1&, float>, int);
    ENSURE_SAME(invoke_result<decltype(&class1::f5), class1&&, float>, int);
    ENSURE_SAME(std::invoke_result_t<decltype(&class1::f5), class1, float>, int);
    ENSURE_SAME(std::invoke_result_t<decltype(&class1::f5), class1&, float>, int);
    ENSURE_SAME(std::invoke_result_t<decltype(&class1::f5), class1&&, float>, int);

    ENSURE_SAME(invoke_result<decltype(&class1::f6), const class1, float>, int);
    ENSURE_SAME(invoke_result<decltype(&class1::f6), class1, float>, int);
    ENSURE_SAME(invoke_result<decltype(&class1::f6), const class1&, float>, int);
    ENSURE_SAME(invoke_result<decltype(&class1::f6), class1&, float>, int);
    ENSURE_SAME(invoke_result<decltype(&class1::f6), const class1&&, float>, int);
    ENSURE_SAME(invoke_result<decltype(&class1::f6), class1&&, float>, int);
    ENSURE_SAME(std::invoke_result_t<decltype(&class1::f6), const class1, float>, int);
    ENSURE_SAME(std::invoke_result_t<decltype(&class1::f6), class1, float>, int);
    ENSURE_SAME(std::invoke_result_t<decltype(&class1::f6), const class1&, float>, int);
    ENSURE_SAME(std::invoke_result_t<decltype(&class1::f6), class1&, float>, int);
    ENSURE_SAME(std::invoke_result_t<decltype(&class1::f6), const class1&&, float>, int);
    ENSURE_SAME(std::invoke_result_t<decltype(&class1::f6), class1&&, float>, int);

    ENSURE_SAME(invoke_result<decltype(&class1::f7), class1&, float>, int);
    ENSURE_SAME(std::invoke_result_t<decltype(&class1::f7), class1&, float>, int);

    ENSURE_SAME(invoke_result<decltype(&class1::f8), const class1&, float>, int);
    ENSURE_SAME(invoke_result<decltype(&class1::f8), class1&, float>, int);
    ENSURE_SAME(invoke_result<decltype(&class1::f8), class1, float>, int);
    ENSURE_SAME(invoke_result<decltype(&class1::f8), const class1, float>, int);
    ENSURE_SAME(invoke_result<decltype(&class1::f8), class1&&, float>, int);
    ENSURE_SAME(invoke_result<decltype(&class1::f8), const class1&&, float>, int);
    ENSURE_SAME(std::invoke_result_t<decltype(&class1::f8), const class1&, float>, int);
    ENSURE_SAME(std::invoke_result_t<decltype(&class1::f8), class1&, float>, int);
    ENSURE_SAME(std::invoke_result_t<decltype(&class1::f8), class1, float>, int);
    ENSURE_SAME(std::invoke_result_t<decltype(&class1::f8), const class1, float>, int);
    ENSURE_SAME(std::invoke_result_t<decltype(&class1::f8), class1&&, float>, int);
    ENSURE_SAME(std::invoke_result_t<decltype(&class1::f8), const class1&&, float>, int);

    ENSURE_SAME(invoke_result<decltype(&class1::f9), class1&&, float>, int);
    ENSURE_SAME(invoke_result<decltype(&class1::f9), class1, float>, int);
    ENSURE_SAME(std::invoke_result_t<decltype(&class1::f9), class1&&, float>, int);
    ENSURE_SAME(std::invoke_result_t<decltype(&class1::f9), class1, float>, int);

    ENSURE_SAME(invoke_result<decltype(&class1::f10), const class1&&, float>, int);
    ENSURE_SAME(invoke_result<decltype(&class1::f10), class1&&, float>, int);
    ENSURE_SAME(invoke_result<decltype(&class1::f10), class1, float>, int);
    ENSURE_SAME(invoke_result<decltype(&class1::f10), const class1, float>, int);
    ENSURE_SAME(std::invoke_result_t<decltype(&class1::f10), const class1&&, float>, int);
    ENSURE_SAME(std::invoke_result_t<decltype(&class1::f10), class1&&, float>, int);
    ENSURE_SAME(std::invoke_result_t<decltype(&class1::f10), class1, float>, int);
    ENSURE_SAME(std::invoke_result_t<decltype(&class1::f10), const class1, float>, int);

    ENSURE_SAME(invoke_result<decltype(&class1::f11), class1, float>, int);
    ENSURE_SAME(invoke_result<decltype(&class1::f11), class1&, float>, int);
    ENSURE_SAME(invoke_result<decltype(&class1::f11), class1&&, float>, int);
    ENSURE_SAME(std::invoke_result_t<decltype(&class1::f11), class1, float>, int);
    ENSURE_SAME(std::invoke_result_t<decltype(&class1::f11), class1&, float>, int);
    ENSURE_SAME(std::invoke_result_t<decltype(&class1::f11), class1&&, float>, int);

    ENSURE_SAME(invoke_result<decltype(&class1::f12), const class1, float>, int);
    ENSURE_SAME(invoke_result<decltype(&class1::f12), class1, float>, int);
    ENSURE_SAME(invoke_result<decltype(&class1::f12), const class1&, float>, int);
    ENSURE_SAME(invoke_result<decltype(&class1::f12), class1&, float>, int);
    ENSURE_SAME(invoke_result<decltype(&class1::f12), const class1&&, float>, int);
    ENSURE_SAME(invoke_result<decltype(&class1::f12), class1&&, float>, int);
    ENSURE_SAME(std::invoke_result_t<decltype(&class1::f12), const class1, float>, int);
    ENSURE_SAME(std::invoke_result_t<decltype(&class1::f12), class1, float>, int);
    ENSURE_SAME(std::invoke_result_t<decltype(&class1::f12), const class1&, float>, int);
    ENSURE_SAME(std::invoke_result_t<decltype(&class1::f12), class1&, float>, int);
    ENSURE_SAME(std::invoke_result_t<decltype(&class1::f12), const class1&&, float>, int);
    ENSURE_SAME(std::invoke_result_t<decltype(&class1::f12), class1&&, float>, int);


    ENSURE_SAME(invoke_result<decltype([]{ return 2; })>, int);
    ENSURE_SAME(std::invoke_result_t<decltype([]{ return 2; })>, int);
}

#include <core/traits/copy_cv.hpp>
using core::copy_cv;

TEST_CASE("copy_cv") {
    ENSURE_SAME(copy_cv<const volatile void, void>, const volatile void);
    ENSURE_SAME(copy_cv<const int, volatile void>, const volatile void);
    ENSURE_SAME(copy_cv<int, const volatile void>, const volatile void);
    ENSURE_SAME(copy_cv<int, void>, void);
}

#include <core/traits/copy_cvref.hpp>
using core::copy_cvref;

TEST_CASE("copy_cvref") {
    ENSURE_SAME(copy_cvref<const volatile int&&, long>, const volatile long&&);
    ENSURE_SAME(copy_cvref<const volatile int* const&, long>, const long&);
    ENSURE_SAME(copy_cvref<int, long>, long);
    ENSURE_SAME(copy_cvref<int&&, long>, long&&);
}

#include <core/traits/common_type.hpp>
using core::common_type;
using core::common_type_s;
using core::have_common_type;

template <typename T>
struct ct_test1 {};

template <typename T>
struct ct_test2 {};

template <>
struct core::common_type_s<ct_test1<int>, ct_test1<int>> {};

template <typename T>
struct core::common_type_s<T, ct_test2<T>> {
    using type = ct_test2<T>;
};

template <typename T>
struct core::common_type_s<ct_test2<T>, T> {
    using type = ct_test2<T>;
};

template <typename T>
struct core::common_type_s<ct_test2<T>, ct_test2<T>> {
    using type = ct_test2<T>;
};

template <>
struct core::common_type_s<ct_test2<long>, long> {};

template <>
struct core::common_type_s<long, ct_test2<long>> {};

template <typename T>
struct deleted_mctor {
    deleted_mctor(T&);
    deleted_mctor(T&&) = delete;
    operator T&() const;
};

using core::type;
using core::type_t;

TEST_CASE("common_type") {
    ENSURE_SAME(common_type<deleted_mctor<int>, const int&>, int);
    ENSURE_SAME(common_type<const volatile deleted_mctor<int>&, const int&, const volatile int>, int);
    static_assert(!core::have_common_type<>);

    ENSURE_SAME(common_type<void>, void);
    ENSURE_SAME(common_type<int>, int);
    ENSURE_SAME(common_type<const int>, int);
    ENSURE_SAME(common_type<int volatile[]>, int volatile*);
    ENSURE_SAME(common_type<void (&)()>, void (*)());
    static_assert(!have_common_type<const volatile ct_test1<int>&&>);
    ENSURE_SAME(common_type<const volatile ct_test1<float>&&>, ct_test1<float>);

    auto test3 = []<typename T, typename U, typename Expect>(type_t<T>, type_t<U>, type_t<Expect>) {
        static_assert(!is_same<T, decay<T>> || !is_same<U, decay<U>>);
        ENSURE_SAME(common_type<T, U>, Expect);
        ENSURE_SAME(common_type<U, T>, Expect);
        ENSURE_SAME(common_type<T, U>, common_type<decay<T>, decay<U>>);
    };

    test3(type<const ct_test2<int>>, type<int>, type<ct_test2<int>>);
    static_assert(!have_common_type<const ct_test2<long>, long>);
    static_assert(!have_common_type<long, const ct_test2<long>>);
    test3(type<const void>, type<const void>, type<void>);
    test3(type<const int[]>, type<const int[]>, type<const int*>);
    test3(type<const int(&)[]>, type<const int(&)[]>, type<const int*>);
    test3(type<const int[]>, type<const int(&)[]>, type<const int*>);

    ENSURE_SAME(common_type<const int*, int*>, const int*);
    static_assert(!have_common_type<int, void>);
    static_assert(!have_common_type<int, ct_test1<int>>);
    ENSURE_SAME(common_type<void, void>, void);
    ENSURE_SAME(common_type<void, void>, common_type<void>);

    ENSURE_SAME(common_type<int, deleted_mctor<int>>, int);
    ENSURE_SAME(common_type<deleted_mctor<double>, double>, double);
    ENSURE_SAME(common_type<const deleted_mctor<int>, int>, int);
    ENSURE_SAME(common_type<volatile deleted_mctor<int>, int>, int);
    ENSURE_SAME(common_type<const volatile deleted_mctor<int>, int>, int);
    ENSURE_SAME(common_type<deleted_mctor<int>, const int>, int);
    ENSURE_SAME(common_type<deleted_mctor<int>, volatile int>, int);
    ENSURE_SAME(common_type<deleted_mctor<int>, const volatile int>, int);
    ENSURE_SAME(common_type<deleted_mctor<int>&, int>, int);
    ENSURE_SAME(common_type<deleted_mctor<int>, int&>, int);

    struct some {};
    static_assert(!have_common_type<int, some>);
    static_assert(!have_common_type<int, int, some>);
    static_assert(!have_common_type<int, int, some, int>);
    static_assert(!have_common_type<int, int, int, some>);
}

#if 0

#include <core/traits/common_ref.hpp>

using core::common_ref;
using core::have_common_ref;

TEST_CASE("common_ref") {
    static_assert(!have_common_ref<>);

    ENSURE_SAME(common_ref<void>, void);
    ENSURE_SAME(common_ref<int>, int);
    ENSURE_SAME(common_ref<int&>, int&);
    ENSURE_SAME(common_ref<int&&>, int&&);
    ENSURE_SAME(common_ref<int const>, int const);
    ENSURE_SAME(common_ref<int const&>, int const&);
    ENSURE_SAME(common_ref<int const&&>, int const&&);
    ENSURE_SAME(common_ref<int volatile[]>, int volatile[]);
    ENSURE_SAME(common_ref<int volatile(&)[]>, int volatile(&)[]);
    ENSURE_SAME(common_ref<int volatile(&&)[]>, int volatile(&&)[]);
    ENSURE_SAME(common_ref<void (&)()>, void (&)());
    ENSURE_SAME(common_ref<void (&&)()>, void(&&)());

    struct B {};
    struct D : B {};
    ENSURE_SAME(common_ref<B&, D&>, B&);
    ENSURE_SAME(common_ref<B const&, D&>, B const&);
    ENSURE_SAME(common_ref<B&, D const&>, B const&);
    ENSURE_SAME(common_ref<B&, D const&, D&>, B const&);
    ENSURE_SAME(common_ref<B&, D&, B&, D&>, B&);

    ENSURE_SAME(common_ref<B&&, D&&>, B&&);
    ENSURE_SAME(common_ref<B const&&, D&&>, B const&&);
    ENSURE_SAME(common_ref<B&&, D const&&>, B const&&);
    ENSURE_SAME(common_ref<B&, D&&>, B const&);
    ENSURE_SAME(common_ref<B&, D const&&>, B const&);
    ENSURE_SAME(common_ref<B const&, D&&>, B const&);
    ENSURE_SAME(common_ref<B&&, D&>, B const&);
    ENSURE_SAME(common_ref<B&&, D const&>, B const&);
    ENSURE_SAME(common_ref<B const&&, D&>, B const&);
    ENSURE_SAME(common_ref<int const&, int volatile&>, int const volatile&);
    ENSURE_SAME(common_ref<int const volatile&&, int volatile&&>, int const volatile&&);
    ENSURE_SAME(common_ref<int(&)[10], int(&&)[10]>, int const(&)[10]);
    ENSURE_SAME(common_ref<int const(&)[10], int volatile(&)[10]>, int const volatile(&)[10]);

    // static_assert(std::is_same_v<std::common_reference_t<const UserTuple<int, short>&, UserTuple<int&, short
    // volatile&>>,
    //                              UserTuple<const int&, const volatile short&>>);
    //
    // static_assert(std::is_same_v<std::common_reference_t<volatile UserTuple<int, short>&, const UserTuple<int,
    // short>&>,
    //                              const volatile UserTuple<int, short>&>);

    ENSURE_SAME(common_ref<void, void>, void);
    ENSURE_SAME(common_ref<int, short>, int);
    ENSURE_SAME(common_ref<int, short&>, int);
    ENSURE_SAME(common_ref<int&, short&>, int);
    ENSURE_SAME(common_ref<int&, short>, int);
    ENSURE_SAME(common_ref<int&&, int volatile&>, int);
    ENSURE_SAME(common_ref<int volatile&, int&&>, int);
    ENSURE_SAME(common_ref<int(&)[10], int(&)[11]>, int*);

    // https://github.com/ericniebler/stl2/issues/338
    struct int_ref {
        int_ref(int&);
    };
    ENSURE_SAME(common_ref<int&, int_ref>, int_ref);

    struct moveonly {
        moveonly()                      = default;
        moveonly(moveonly&&)            = default;
        moveonly& operator=(moveonly&&) = default;
    };
    struct moveonly2 : moveonly {};

    ENSURE_SAME(common_ref<moveonly const&, moveonly>, moveonly);
    ENSURE_SAME(common_ref<moveonly2 const&, moveonly>, moveonly);
    ENSURE_SAME(common_ref<moveonly const&, moveonly2>, moveonly);

    //static_assert(std::is_same_v<std::common_reference_t<X2&, Y2 const&>, Z2>);

    //static_assert(!has_type<std::common_reference<volatile UserTuple<short>&, const UserTuple<int, short>&>>);

    ENSURE_SAME(common_ref<int, int, int>, int);
    ENSURE_SAME(common_ref<int&&, int const&, int volatile&>, int const volatile&);
    ENSURE_SAME(common_ref<int&&, int const&, float&>, float);

    // (6.4.2)
    //    -- Otherwise, there shall be no member type.
    static_assert(!have_common_ref<int, short, int, char*>);

    static_assert(std::is_same_v<std::common_reference_t<std::tuple<int, int>>, std::tuple<int, int>>);
    static_assert(
        std::is_same_v<std::common_reference_t<std::tuple<int, long>, std::tuple<long, int>>, std::tuple<long, long>>);
    static_assert(std::is_same_v<std::common_reference_t<std::tuple<int&, const int&>, std::tuple<const int&, int>>,
                                 std::tuple<const int&, int>>);
    static_assert(
        std::is_same_v<std::common_reference_t<std::tuple<int&, volatile int&>, std::tuple<volatile int&, int>>,
                       std::tuple<volatile int&, int>>);
    static_assert(std::is_same_v<
                  std::common_reference_t<std::tuple<int&, const volatile int&>, std::tuple<const volatile int&, int>>,
                  std::tuple<const volatile int&, int>>);
    static_assert(
        !has_type<
            std::common_reference_t<std::tuple<const int&, volatile int&>, std::tuple<volatile int&, const int&>>>);

    static_assert(
        std::is_same_v<std::common_reference_t<std::tuple<int, X2>, std::tuple<int, Y2>>, std::tuple<int, Z2>>);
    static_assert(
        std::is_same_v<std::common_reference_t<std::tuple<int, X2>, std::tuple<int, Y2>>, std::tuple<int, Z2>>);
    static_assert(!has_type<std::common_reference<std::tuple<int, const X2>, std::tuple<float, const Z2>>>);
    static_assert(!has_type<std::common_reference<std::tuple<int, X2>, std::tuple<float, Z2>>>);
    static_assert(!has_type<std::common_reference<std::tuple<int, X2>, int, X2>>);

    struct A {};
    template <template <class> class TQual, template <class> class UQual>
    struct std::basic_common_reference<A, std::tuple<B>, TQual, UQual> {
        using type = tuple<UQual<B>>;
    };

    static_assert(std::is_same_v<std::common_reference_t<A, std::tuple<B>, std::tuple<D>>, std::tuple<B>>);

    static_assert(std::is_same_v<std::common_reference_t<std::pair<int, int>>, std::pair<int, int>>);
    static_assert(
        std::is_same_v<std::common_reference_t<std::pair<int, long>, std::pair<long, int>>, std::pair<long, long>>);
    static_assert(std::is_same_v<std::common_reference_t<std::pair<int&, const int&>, std::pair<const int&, int>>,
                                 std::pair<const int&, int>>);
    static_assert(std::is_same_v<std::common_reference_t<std::pair<int&, volatile int&>, std::pair<volatile int&, int>>,
                                 std::pair<volatile int&, int>>);
    static_assert(std::is_same_v<
                  std::common_reference_t<std::pair<int&, const volatile int&>, std::pair<const volatile int&, int>>,
                  std::pair<const volatile int&, int>>);
    static_assert(
        !has_type<std::common_reference_t<std::pair<const int&, volatile int&>, std::pair<volatile int&, const int&>>>);

    static_assert(std::is_same_v<std::common_reference_t<std::pair<int, X2>, std::pair<int, Y2>>, std::pair<int, Z2>>);
    static_assert(std::is_same_v<std::common_reference_t<std::pair<int, X2>, std::pair<int, Y2>>, std::pair<int, Z2>>);
    static_assert(!has_type<std::common_reference<std::pair<int, const X2>, std::pair<float, const Z2>>>);
    static_assert(!has_type<std::common_reference<std::pair<int, X2>, std::pair<float, Z2>>>);
    static_assert(!has_type<std::common_reference<std::pair<int, X2>, int, X2>>);
}

#endif
