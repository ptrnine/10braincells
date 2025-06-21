#pragma once

#include <core/utility/move.hpp>
#include <core/ranges/range.hpp>

namespace core {
namespace details {
    template <typename DerivedT, typename F, typename I>
    struct inward_iterator_derived {
        inward_iterator_derived(I iter, F operation): i(core::mov(iter)), op(core::mov(operation)) {}

        auto operator*() {
            return *i | op;
        }

        auto operator->() {
            return *i | op;
        }

        auto operator*() const {
            return *i | op;
        }

        auto operator->() const {
            return *i | op;
        }

        DerivedT& operator++() {
            ++i;
            return static_cast<DerivedT&>(*this);
        }

        DerivedT operator++(int) {
            auto res = *this;
            ++(*this);
            return static_cast<DerivedT>(res);
        }

        [[nodiscard]]
        friend bool operator==(const DerivedT& rhs, const DerivedT& lhs) {
            return rhs.i == lhs.i;
        }

        I i;
        F op;

        using value_type = std::decay_t<decltype(*i | op)>;
    };
} // namespace details

template <typename F, typename I>
struct inward_iterator_input : public details::inward_iterator_derived<inward_iterator_input<F, I>, F, I> {
    using details::inward_iterator_derived<inward_iterator_input<F, I>, F, I>::inward_iterator_derived;
};

template <typename F, typename I>
struct inward_iterator_bidirect : public details::inward_iterator_derived<inward_iterator_bidirect<F, I>, F, I> {
    using details::inward_iterator_derived<inward_iterator_bidirect<F, I>, F, I>::inward_iterator_derived;

    inward_iterator_bidirect& operator--() {
        --this->i;
        return *this;
    }

    inward_iterator_bidirect operator--(int) {
        auto res = *this;
        --(*this);
        return res;
    }
};

template <typename F>
struct inward {
    inward(F operation): _op(core::mov(operation)) {}

    template <input_iterator I>
    auto operator()(I begin, I end) {
        return range_holder{inward_iterator_input<F, decltype(begin)>(begin, _op), inward_iterator_input<F, decltype(end)>(end, _op)};
    }

    template <bidirect_iterator I>
    auto operator()(I begin, I end) {
        return range_holder{inward_iterator_bidirect<F, decltype(begin)>(begin, _op), inward_iterator_bidirect<F, decltype(end)>(end, _op)};
    }

    friend auto operator|(range auto&& container, inward inward) {
        return inward(begin(container), end(container));
    }

    F _op;
};
} // namespace core
