#pragma once

#include "core/array.hpp"
#include <core/begin_end.hpp>
#include <core/traits/decay.hpp>
#include <core/traits/declval.hpp>
#include <core/traits/remove_const.hpp>
#include <core/utility/move.hpp>

#include "range.hpp"

#define fwd(x) static_cast<decltype(x)>(x)

namespace core::views {
template <typename I, size_t Ndelims>
class split_iterator {
public:
    using non_const_t = decay<decltype(*declval<I>())>;
    using value_type = decltype(range_holder{declval<I>(), declval<I>()});
    using reference = value_type;

    split_iterator(I start, I iend, const array<non_const_t, Ndelims>& idelims, bool iallow_empty = false):
        b(start), e(iend), end(iend), delims(idelims), allow_empty(iallow_empty) {
        init();
    }

    split_iterator& operator++() {
        next();
        return *this;
    }

    split_iterator operator++(int) {
        auto result = *this;
        ++(*this);
        return result;
    }

    auto operator*() {
        return range_holder{b, e};
    }

    [[nodiscard]]
    friend bool operator==(const split_iterator& rhs, const split_iterator& lhs) {
        //assert(rhs.delims == lhs.delims && rhs.allow_empty == lhs.allow_empty);
        return rhs.b == lhs.b;
    }

private:
    static bool has_delim(const array<non_const_t, Ndelims>& delims, const non_const_t& c) {
        for (auto&& delim : delims)
            if (delim == c)
                return true;
        return false;
    }

    void init() {
        if (!allow_empty)
            while (b != end && has_delim(delims, *b))
                ++b;
        e = b;
        while (e != end && !has_delim(delims, *e))
            ++e;
    }

    void next() {
        bool repeat = true;

        while (repeat) {
            b = e;
            if (b == end)
                return;

            e = ++b;
            while (e != end && !has_delim(delims, *e))
                ++e;

            repeat = !allow_empty && b == e;
        }
    }

private:
    I                           b, e, end;
    array<non_const_t, Ndelims> delims;
    bool                        allow_empty = false;
};

template <typename I, size_t Ndelims>
class split_viewer {
public:
    using value_type = remove_ref<decltype(*declval<I>())>;
    using T = remove_const_ref<value_type>;

    split_viewer(range auto&& container, const array<T, Ndelims>& delims, bool allow_empty = false):
        b(core::begin(container), core::end(container), delims, allow_empty),
        e(core::end(container), core::end(container), delims, allow_empty) {}

    split_iterator<I, Ndelims> begin() {
        return b;
    }

    split_iterator<I, Ndelims> end() {
        return e;
    }

private:
    split_iterator<I, Ndelims> b, e;
};

enum class split_mode : uint8_t {
    skip_empty,
    allow_empty
};

template <typename ContainerT, typename T, size_t Ndelims>
split_viewer(ContainerT&&, const array<T, Ndelims>&, bool)
    -> split_viewer<decltype(core::begin(declval<ContainerT>())), Ndelims>;

template <typename T, size_t Ndelims>
struct split {
    template <typename C, typename... Cs>
    split(C delim, Cs... delims):
        _delims{mov(delim), mov(delims)...}, _mode(split_mode::skip_empty) {}

    template <typename C, typename... Cs>
    split(split_mode mode, C delim, Cs... delims):
        _delims{mov(delim), mov(delims)...}, _mode(mode) {}

    friend auto operator|(range auto&& container, const split& sv) {
        return split_viewer(fwd(container), sv._delims, sv._mode == split_mode::allow_empty);
    }

    array<T, Ndelims> _delims;
    split_mode        _mode;
};

template <typename C, typename... Cs>
split(C, Cs...) -> split<C, sizeof...(Cs) + 1>;

template <typename C, typename... Cs>
split(split_mode, C, Cs...) -> split<C, sizeof...(Cs) + 1>;


template <typename I, typename F>
class split_if_iterator {
public:
    using non_const_t = decay<decltype(*declval<I>())>;

    using value_type = decltype(range_holder{declval<I>(), declval<I>()});
    using reference = value_type;

    split_if_iterator(I start, I iend, F binary_op, bool iallow_empty = false):
        b(start), e(iend), end(iend), _op(mov(binary_op)), allow_empty(iallow_empty) {
        init();
    }

    split_if_iterator& operator++() {
        next();
        return *this;
    }

    split_if_iterator operator++(int) {
        auto result = *this;
        ++(*this);
        return result;
    }

    auto operator*() {
        return range_holder{b, e};
    }

    [[nodiscard]]
    friend bool operator==(const split_if_iterator& rhs, const split_if_iterator& lhs) {
        //assert(rhs.allow_empty == lhs.allow_empty);
        return rhs.b == lhs.b;
    }

private:
    void init() {
        if (!allow_empty)
            while (b != end && _op(*b))
                ++b;
        e = b;
        while (e != end && !_op(*e))
            ++e;
    }

    void next() {
        bool repeat = true;

        while (repeat) {
            b = e;
            if (b == end)
                return;

            e = ++b;
            while (e != end && !_op(*e))
                ++e;

            repeat = !allow_empty && b == e;
        }
    }

private:
    I b, e, end;
    F _op;
    bool allow_empty = false;
};


template <typename I, typename F>
class split_if_viewer {
public:
    using value_type = remove_ref<decltype(*declval<I>())>;
    using T          = remove_const<value_type>;

    split_if_viewer(range auto&& container, F binary_op, bool allow_empty = false):
        b(core::begin(container), core::end(container), mov(binary_op), allow_empty),
        e(core::end(container), core::end(container), mov(binary_op), allow_empty) {}

    split_if_iterator<I, F> begin() {
        return b;
    }

    split_if_iterator<I, F> end() {
        return e;
    }

private:
    split_if_iterator<I, F> b, e;
};

template <typename ContainerT, typename F>
split_if_viewer(ContainerT&&,
                  F,
                  bool) -> split_if_viewer<decltype(begin(declval<ContainerT>())), F>;

template <typename F>
struct split_if {
    split_if(F binary_op):
        _op(mov(binary_op)), _mode(split_mode::skip_empty) {}

    split_if(split_mode mode, F binary_op):
        _op(mov(binary_op)), _mode(mode) {}

    friend auto operator|(range auto&& container, const split_if& sv) {
        return split_if_viewer(fwd(container), sv._op, sv._mode == split_mode::allow_empty);
    }

    F _op;
    split_mode _mode;
};

} // namespace core

/*
template <typename T, size_t Ndelims>
struct std::iterator_traits<core::views::split_iterator<T, Ndelims>> {
    using value_type = typename core::views::split_iterator<T, Ndelims>::value_type;
    using reference = value_type;
    using difference_type = ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;
};

template <typename T, typename F>
struct std::iterator_traits<core::views::split_if_iterator<T, F>> {
    using value_type = typename core::views::split_if_iterator<T, F>::value_type;
    using reference = value_type;
    using difference_type = ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;
};
*/

#undef fwd
