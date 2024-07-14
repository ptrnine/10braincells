#pragma once

#include <span>
#include <string_view>

#include <core/begin_end.hpp>
#include <core/concepts/any_of.hpp>
#include <core/concepts/convertible_to.hpp>
#include <core/traits/declval.hpp>
#include <core/traits/is_ptr.hpp>
#include <core/traits/remove_cvref.hpp>

namespace core
{
template <typename T>
concept range = requires(T&& v) {
    begin(v);
    end(v);
};

template <typename T>
concept input_iterator = requires(T v) {
    {v++};
    {++v};
    {*v};
    { v == v } -> convertible_to<bool>;
    { v != v } -> convertible_to<bool>;
};

template <typename T>
concept input_range = range<T> && requires(T&& v) {
    requires input_iterator<decltype(begin(v))>;
};

template <typename T>
concept bidirect_iterator = input_iterator<T> && requires(T v) {
    {v--};
    {--v};
};

template <typename T>
concept bidirect_range = input_range<T> && requires(T&& v) {
    requires bidirect_iterator<decltype(begin(v))>;
};

template <typename T>
concept random_access_iterator = bidirect_iterator<T> && requires(T v) {
    {v - v} -> convertible_to<diff_t>;
};

template <typename T>
concept random_access_range = bidirect_range<T> && requires(T&& v) {
    { end(v) - begin(v) } -> convertible_to<diff_t>;
};

namespace detail {
    template <typename F, typename I>
    concept RangeFunc = requires (F func, I iter) {
        func(iter, iter);
    };
}

template <typename I1, typename I2>
struct range_base {
    range_base() = default;
    range_base(I1 ibegin, I2 iend): _begin(mov(ibegin)), _end(mov(iend)) {}

    constexpr auto begin(this auto&& it) {
        return it._begin;
    }

    constexpr auto end(this auto&& it) {
        return it._end;
    }

    I1 _begin;
    I2 _end;
};

template <typename I1, typename I2>
struct range_holder : range_base<I1, I2> {
    using range_base<I1, I2>::range_base;
};

template <input_iterator I1, typename I2>
struct range_holder<I1, I2> : range_base<I1, I2> {
    using range_base<I1, I2>::range_base;
};

template <random_access_iterator I1, typename I2>
struct range_holder<I1, I2> : range_base<I1, I2> {
    using range_base<I1, I2>::range_base;

    constexpr size_t size() const {
        return size_t(this->end() - this->begin());
    }
};

namespace details
{
    template <typename T>
    concept char_type =
        same_as<remove_cvref<T>, char> || same_as<remove_cvref<T>, wchar_t> || same_as<remove_cvref<T>, char16_t> ||
        same_as<remove_cvref<T>, char32_t> || same_as<remove_cvref<T>, char8_t>;

    template <typename I1, typename I2>
    concept string_view_ctor = requires (I1 i) { {*i} -> char_type; } && requires (I1 i1, I2 i2) {
        std::basic_string_view<remove_cvref<decltype(*i1)>>(i1, i2);
    };
} // namespace details

template <random_access_iterator I1, typename I2> requires details::string_view_ctor<I1, I2>
struct range_holder<I1, I2> : range_base<I1, I2> {
    using range_base<I1, I2>::range_base;

    using char_t = remove_cvref<decltype(*declval<I1>())>;

    constexpr size_t size() const {
        return size_t(this->end() - this->begin());
    }

    constexpr auto data(this auto&& it) {
        return std::span(it.begin(), it.end()).data();
    }

    constexpr operator std::basic_string_view<char_t>() const {
        return as_string();
    }

    constexpr auto as_string() const {
        return std::basic_string_view<char_t>(this->begin(), this->end());
    }
};

template <random_access_iterator I1, typename I2> requires (!details::string_view_ctor<I1, I2>) && is_ptr<I1> && is_ptr<I2>
struct range_holder<I1, I2> : range_base<I1, I2> {
    using range_base<I1, I2>::range_base;

    constexpr size_t size() const {
        return size_t(this->end() - this->begin());
    }

    constexpr auto data(this auto&& it) {
        return it.begin();
    }
};

template <typename I1, typename I2>
range_holder(I1, I2) -> range_holder<I1, I2>;
} // namespace core
