#pragma once

#include <string>

#include <core/array.hpp>
#include <core/tuple.hpp>

namespace sys
{
/*
 * [[[codegen start]]]
 * Generated with:
 * ./build-deb/codegen/flags remap_flag remap_flags int may_move=0x1 fixed=0x2 dont_unmap=0x4
 */

enum class remap_flag : int {
    may_move   = 0x1,
    fixed      = 0x2,
    dont_unmap = 0x4,
};

struct remap_flags {
    using enum remap_flag;

    constexpr remap_flags() = default;
    constexpr remap_flags(remap_flag ivalue): value(int(ivalue)) {}

    constexpr remap_flags operator|(remap_flags flag) const {
        return remap_flag(int(value | flag.value));
    }

    constexpr remap_flags operator&(remap_flags flag) const {
        return remap_flag(int(value & flag.value));
    }

    constexpr remap_flags& operator|=(remap_flags flag) {
        value = int(value | flag.value);
        return *this;
    }

    constexpr bool test(remap_flags flags) const {
        return value & flags.value;
    }

    explicit constexpr operator bool() const {
        return value;
    }

    constexpr void unset(remap_flags flags) {
        value &= int(~flags.value);
    }

    constexpr std::string to_string() const {
        constexpr core::array flags = {
            core::tuple{remap_flag::may_move, std::string_view("may_move")},
            core::tuple{remap_flag::fixed, std::string_view("fixed")},
            core::tuple{remap_flag::dont_unmap, std::string_view("dont_unmap")},
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

inline constexpr remap_flags operator|(remap_flag lhs, remap_flag rhs) {
    return remap_flag(int(lhs) | int(rhs));
}

/* [[[codegen end]]] */
} // namespace sys
