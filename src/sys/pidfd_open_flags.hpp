/* Command: ./build/codegen/cg_flags name=pidfd_open_flag name_combined=pidfd_open_flags type=int nonblock=04000 */
#pragma once

#include <string>

#include <core/array.hpp>
#include <core/tuple.hpp>

namespace sys {
enum class pidfd_open_flag : int {
    nonblock = 04000,
};

struct pidfd_open_flags {
    using enum pidfd_open_flag;

    constexpr pidfd_open_flags() = default;
    constexpr pidfd_open_flags(pidfd_open_flag ivalue): value(int(ivalue)) {}

    constexpr pidfd_open_flags operator|(pidfd_open_flags flag) const {
        return pidfd_open_flag(int(value | flag.value));
    }

    constexpr pidfd_open_flags operator&(pidfd_open_flags flag) const {
        return pidfd_open_flag(int(value & flag.value));
    }

    constexpr pidfd_open_flags& operator|=(pidfd_open_flags flag) {
        value = int(value | flag.value);
        return *this;
    }

    constexpr bool test(pidfd_open_flags flags) const {
        return value & flags.value;
    }

    explicit constexpr operator bool() const {
        return value;
    }

    constexpr void unset(pidfd_open_flags flags) {
        value &= int(~flags.value);
    }

    constexpr std::string to_string() const {
        constexpr core::array flags = {
            core::tuple{pidfd_open_flag::nonblock, std::string_view("nonblock")},
        };
        std::string res;
        for (auto&& [f, s] : flags) {
            if (int(value) & int(f)) {
                res.append(s);
                res.append(" | ");
            }
        }
        if (res.size() > 2)
            res.resize(res.size() - 2);
        return res;
    }

    int value;
};

inline constexpr pidfd_open_flags operator|(pidfd_open_flag lhs, pidfd_open_flag rhs) {
    return pidfd_open_flag(int(lhs) | int(rhs));
}
} // namespace sys
