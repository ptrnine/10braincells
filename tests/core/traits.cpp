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

