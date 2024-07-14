#pragma once

#include <core/ranges/range.hpp>
#include <core/traits/remove_cvref.hpp>
#include <core/utility/move.hpp>

#define fwd(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace core
{
template <typename I>
struct fold_iterator_sentinel {
    I iter;
};

template <typename I, typename T> requires requires (I& i, const T& t) { true ? *i : t; }
class fold_iterator {
public:
    constexpr fold_iterator(I iterator, T delimiter): iter(mov(iterator)), delim(mov(delimiter)) {}

    constexpr fold_iterator& operator++() {
        if (!on_delim)
            ++iter;
        on_delim = !on_delim;
        return *this;
    }

    constexpr fold_iterator operator++(int) {
        auto tmp = *this;
        ++(*this);
        return tmp;
    }

    template <typename U>
    constexpr bool operator==(const fold_iterator_sentinel<U>& i) const {
        return iter == i.iter;
    }

    template <typename U>
    constexpr bool operator!=(const fold_iterator_sentinel<U>& i) const {
        return !(*this == i);
    }

    auto operator*() {
        return on_delim ? delim : *iter;
    }

private:
    I    iter;
    T    delim;
    bool on_delim = false;
};
} // namespace core

namespace core::views
{
template <typename T>
class fold {
public:
    fold(T delim): _delim(mov(delim)) {}

    friend auto operator|(range auto&& range, const fold& f) {
        return range_holder{
            fold_iterator{range.begin(), f._delim},
            fold_iterator_sentinel{range.end()},
        };
    }

private:
    T _delim;
};
} // namespace core::views

#undef fwd
