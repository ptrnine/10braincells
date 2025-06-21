#pragma once

#include <compare>
#include <core/basic_types.hpp>

namespace core {
struct index_iterator {
    constexpr index_iterator& operator++() noexcept {
        ++_value;
        return *this;
    }

    constexpr index_iterator& operator--() noexcept {
        --_value;
        return *this;
    }

    constexpr index_iterator operator++(int) noexcept {
        auto tmp = *this;
        ++_value;
        return tmp;
    }

    constexpr index_iterator operator--(int) noexcept {
        auto tmp = *this;
        --_value;
        return tmp;
    }

    constexpr index_iterator& operator+=(size_t size) noexcept {
        _value += size;
        return *this;
    }

    constexpr index_iterator& operator-=(size_t size) noexcept {
        _value -= size;
        return *this;
    }

    constexpr index_iterator operator+(size_t size) const noexcept {
        auto res = *this;
        res += size;
        return *this;
    }

    constexpr index_iterator operator-(size_t size) const noexcept {
        auto res = *this;
        res -= size;
        return *this;
    }

    constexpr diff_t operator-(const index_iterator& i) const noexcept {
        return diff_t(_value) - diff_t(i._value);
    }

    constexpr auto operator<=>(const index_iterator& i) const noexcept = default;

    constexpr const size_t& operator*() const noexcept {
        return _value;
    }

    size_t _value = 0;
};
} // namespace core
