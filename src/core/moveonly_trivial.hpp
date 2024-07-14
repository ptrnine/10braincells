#pragma once

namespace core
{
template <typename T, T DefaultValue>
class moveonly_trivial {
public:
    constexpr moveonly_trivial() = default;
    constexpr moveonly_trivial(const T& value): val(value) {}

    constexpr moveonly_trivial(const moveonly_trivial& m)            = delete;
    constexpr moveonly_trivial& operator=(const moveonly_trivial& m) = delete;

    constexpr moveonly_trivial(moveonly_trivial&& m) noexcept: val(m.val) {
        m.reset();
    }
    constexpr moveonly_trivial& operator=(moveonly_trivial&& m) noexcept {
        val   = m.val;
        m.reset();
        return *this;
    }

    constexpr moveonly_trivial& operator=(const T& value) {
        val = value;
        return *this;
    }

    constexpr operator T() const {
        return val;
    }

    constexpr bool not_default() {
        return val != DefaultValue;
    }

    constexpr void reset() {
        val = DefaultValue;
    }

    constexpr auto& get(this auto&& it) {
        return it.val;
    }

private:
    T val = DefaultValue;
};
} // namespace core
