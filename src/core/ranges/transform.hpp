#pragma once

#include <core/ranges/index.hpp>
#include <core/ranges/range.hpp>
#include <core/utility/move.hpp>

#define fwd(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace core {
template <input_iterator I, typename F>
struct transform_iterator_input {
    transform_iterator_input(I iter, F func): _iterator(core::mov(iter)), _function(core::mov(func)) {}

    auto& operator++(this auto& it) {
        ++it._iterator;
        return it;
    }

    auto operator++(this auto&& it, int) {
        auto i = it;
        ++it._iterator;
        return i;
    }

    bool operator==(const transform_iterator_input& i) const {
        return _iterator == i._iterator;
    }

    bool operator!=(const transform_iterator_input& i) const {
        return _iterator != i._iterator;
    }

    decltype(auto) operator*(this auto&& it) {
        return fwd(it)._function(*fwd(it)._iterator);
    }

    I _iterator;
    F _function;

    using value_type     = remove_cvref<decltype(_function(*_iterator))>;
    using reference_type = decltype(_function(*_iterator));
};

template <bidirect_iterator I, typename F>
struct transform_iterator_bidirect : transform_iterator_input<I, F> {
    using transform_iterator_input<I, F>::transform_iterator_input;

    auto& operator--(this auto&& it) {
        --it._iterator;
        return it;
    }

    auto operator--(this auto&& it, int) {
        auto i = it;
        --(it);
        return i;
    }
};

template <random_access_iterator I, typename F>
struct transform_iterator_random_access : transform_iterator_bidirect<I, F> {
    using transform_iterator_bidirect<I, F>::transform_iterator_bidirect;

    auto& operator+=(this auto& it, diff_t num) {
        it._iterator += num;
        return it;
    }

    auto& operator-=(this auto& it, diff_t num) {
        it._iterator -= num;
        return it;;
    }

    auto operator+(this const auto& it, diff_t num) {
        auto res = it;
        res += num;
        return res;
    }

    auto operator-(this const auto& it, diff_t num) {
        auto res = it;
        res -= num;
        return res;
    }

    diff_t operator-(const transform_iterator_random_access& i) const {
        return this->_iterator - i._iterator;
    }

    bool operator<(const transform_iterator_random_access& i) const {
        return this->_iterator < i._iterator;
    }

    bool operator<=(const transform_iterator_random_access& i) const {
        return this->_iterator <= i._iterator;
    }

    bool operator>(const transform_iterator_random_access& i) const {
        return this->_iterator > i._iterator;
    }

    bool operator>=(const transform_iterator_random_access& i) const {
        return this->_iterator >= i._iterator;
    }
};

template <input_iterator T, typename F>
struct transform_iterator : transform_iterator_input<T, F> {
    using transform_iterator_input<T, F>::transform_iterator_input;
    using tag = std::input_iterator_tag;
};

template <bidirect_iterator T, typename F>
struct transform_iterator<T, F> : transform_iterator_bidirect<T, F> {
    using transform_iterator_bidirect<T, F>::transform_iterator_bidirect;
    using tag = std::bidirectional_iterator_tag;
};

template <random_access_iterator T, typename F>
struct transform_iterator<T, F> : transform_iterator_random_access<T, F> {
    using transform_iterator_random_access<T, F>::transform_iterator_random_access;
    using tag = std::random_access_iterator_tag;
};

template <typename F>
struct transform {
    friend auto operator|(const input_range auto& range, const transform& t) {
        using IB = decltype(core::begin(range));
        using IE = decltype(core::end(range));
        return range_holder{
            transform_iterator<IB, F>{core::begin(range), t.transform_function},
            transform_iterator<IE, F>{core::end(range), t.transform_function},
        };
    }

    F transform_function;
};
} // namespace core

template <typename I, typename F>
struct std::iterator_traits<core::transform_iterator<I, F>> {
    using value_type        = core::transform_iterator<I, F>::value_type;
    using reference         = core::transform_iterator<I, F>::reference_type;
    using difference_type   = ptrdiff_t;
    using iterator_category = core::transform_iterator<I, F>::tag;
};

namespace core {
auto filled_with(size_t count, auto&& init_handler) {
    return sequence(count) | transform{[h = fwd(init_handler)](size_t) mutable {
               return h();
           }};
}
} // namespace core

#undef fwd
