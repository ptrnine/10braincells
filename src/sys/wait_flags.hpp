/* Command: ./build/codegen/cg_flags name=wait_flag name_combined=wait_flags type=int namespace_name=sys no_hang=0x00000001 stopped=0x00000002 exited=0x00000004 continued=0x00000008 no_wait=0x01000000
 */
#pragma once

#include <string>

#include <core/array.hpp>
#include <core/tuple.hpp>

namespace sys {
enum class wait_flag : int {
    no_hang = 0x00000001,
    stopped = 0x00000002,
    exited = 0x00000004,
    continued = 0x00000008,
    no_wait = 0x01000000
,
};

struct wait_flags {
    using enum wait_flag;

    constexpr wait_flags() = default;
    constexpr wait_flags(wait_flag ivalue): value(int(ivalue)) {}

    constexpr wait_flags operator|(wait_flags flag) const {
        return wait_flag(int(value | flag.value));
    }

    constexpr wait_flags operator&(wait_flags flag) const {
        return wait_flag(int(value & flag.value));
    }

    constexpr wait_flags& operator|=(wait_flags flag) {
        value = int(value | flag.value);
        return *this;
    }

    constexpr bool test(wait_flags flags) const {
        return value & flags.value;
    }

    explicit constexpr operator bool() const {
        return value;
    }

    constexpr void unset(wait_flags flags) {
        value &= int(~flags.value);
    }

    constexpr std::string to_string() const {
        constexpr core::array flags = {
            core::tuple{wait_flag::no_hang, std::string_view("no_hang")},
            core::tuple{wait_flag::stopped, std::string_view("stopped")},
            core::tuple{wait_flag::exited, std::string_view("exited")},
            core::tuple{wait_flag::continued, std::string_view("continued")},
            core::tuple{wait_flag::no_wait, std::string_view("no_wait")},
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

inline constexpr wait_flags operator|(wait_flag lhs, wait_flag rhs) {
    return wait_flag(int(lhs) | int(rhs));
}
} // namespace sys
