#pragma once

#include <core/array.hpp>
#include <core/ranges/range.hpp>
#include <util/basic_types.hpp>
#include <util/vec.hpp>

namespace util {

struct dimensional_index_iterator_sentinel {};

template <typename T, size_t S>
class dimensional_index_iterator {
public:
    constexpr dimensional_index_iterator(const core::array<T, S>& end, uint radix = 0): _size(end), _radix(radix) {}
    constexpr dimensional_index_iterator(const core::array<T, S>& current,
                                         const core::array<T, S>& end,
                                         uint                     radix = 0):
        _current(current), _size(end), _radix(radix) {}

    constexpr dimensional_index_iterator& operator++() {
        increment();
        return *this;
    }

    constexpr dimensional_index_iterator operator++(int) {
        dimensional_index_iterator res = *this;
        increment();
        return res;
    }

    constexpr dimensional_index_iterator& operator--() {
        decrement();
        return *this;
    }

    constexpr dimensional_index_iterator operator--(int) {
        dimensional_index_iterator res = *this;
        decrement();
        return res;
    }

    constexpr bool operator==(const dimensional_index_iterator& i) const noexcept {
        return _current == i._current && _radix == i._radix;
    }

    constexpr bool operator!=(const dimensional_index_iterator& i) const noexcept {
        return !(*this == i);
    }

    constexpr bool operator==(dimensional_index_iterator_sentinel) const noexcept {
        return _radix > 0;
    }

    constexpr bool operator!=(dimensional_index_iterator_sentinel) const noexcept {
        return _radix == 0;
    }
    constexpr vec<T, S> operator*() const noexcept {
        return vec<T, S>{_current};
    }

    constexpr uint radix() const {
        return _radix;
    }

private:
    constexpr void increment(size_t idx = 0) {
        if (idx != S) {
            ++_current[idx];
            if (_current[idx] == _size[idx]) {
                _current[idx] = 0;
                increment(idx + 1);
            }
        }
        else {
            ++_radix;
        }
    }

    constexpr void decrement(size_t idx = 0) {
        if (idx != S) {
            --_current[idx];
            if (_current[idx] > _size[idx]) {
                _current[idx] = _size[idx] - 1;
                decrement(idx + 1);
            }
        }
        else {
            --_radix;
        }
    }

private:
    core::array<T, S> _current = {0};
    core::array<T, S> _size;
    uint              _radix = 0;
};

/**
 * @brief Generate n-dimensional index sequence
 *
 * Usage:
 * for (size_t [x, y] : dimensional_seq(10, 20)) {
 * }
 *
 * Equivalent of:
 * for (size_t y = 0; y < 20; ++y) {
 *     for (size_t x = 0; x < 10; ++x) {
 *     }
 * }
 *
 * @tparam T    - type of index
 * @param x_max - an number specifying at which position to stop for first dimension (not included)
 * @param n_max - another numbers specifying at which position to stop (not included)
 * @return      - n-dimensional index sequence
 *
 */
template <typename T = size_t, typename... Ts>
constexpr auto dimensional_seq(size_t x_max, Ts... n_max) {
    auto max = core::array{x_max, static_cast<T>(n_max)...};
    return core::range_holder{
        dimensional_index_iterator(max),
        dimensional_index_iterator_sentinel{},
    };
}

/**
 * @brief Generate n-dimensional index sequence
 *
 * Usage:
 * for (int [x, y] : dimensional_seq(vec{10, 20})) {
 * }
 *
 * Equivalent of:
 * auto v = vec{10, 20};
 * for (int y = 0; y < vec.y(); ++y) {
 *     for (size_t x = 0; x < vec.x(); ++x) {
 *     }
 * }
 *
 * @tparam T  - type of index
 * @param max - n-dimensional vector specifying at which positions to stop (not included)
 * @return    - n-dimensional index sequence
 *
 */
template <typename T = size_t, size_t S>
constexpr auto dimensional_seq(vec<T, S> maxs) {
    return core::range_holder{
        dimensional_index_iterator(maxs.v),
        dimensional_index_iterator_sentinel{},
    };
}

/**
 * @brief Generate n-dimensional index sequence
 *
 * Usage:
 * for (int [x, y] : dimensional_seq(vec{1, 2}, vec{10, 20})) {
 * }
 *
 * Equivalent of:
 * auto v = vec{10, 20};
 * for (int y = 2; y < vec.y(); ++y) {
 *     for (size_t x = 1; x < vec.x(); ++x) {
 *     }
 * }
 *
 * @tparam T  - type of index
 * @param max - n-dimensional vector specifying at which positions to start (included)
 * @param max - n-dimensional vector specifying at which positions to stop (not included)
 * @return    - n-dimensional index sequence
 *
 */
template <typename T = size_t, size_t S>
constexpr auto dimensional_seq(vec<T, S> start, vec<T, S> maxs) {
    return core::range_holder{
        dimensional_index_iterator(start.v, maxs.v),
        dimensional_index_iterator_sentinel{},
    };
}
} // namespace util
