#pragma once

#include <string>

#include <core/array.hpp>
#include <core/tuple.hpp>

namespace sys
{
/*
 * [[[codegen start]]]
 * Generated with:
 * ./build-deb/codegen/flags map_prot map_prots int none=0x0 read=0x1 write=0x2 exec=0x4
 */

enum class map_prot : int {
    none  = 0x0,
    read  = 0x1,
    write = 0x2,
    exec  = 0x4,
};

struct map_prots {
    using enum map_prot;

    constexpr map_prots() = default;
    constexpr map_prots(map_prot ivalue): value(int(ivalue)) {}

    constexpr map_prots operator|(map_prots flag) const {
        return map_prot(int(value | flag.value));
    }

    constexpr map_prots operator&(map_prots flag) const {
        return map_prot(int(value & flag.value));
    }

    constexpr map_prots& operator|=(map_prots flag) {
        value = int(value | flag.value);
        return *this;
    }

    constexpr bool test(map_prots flags) const {
        return value & flags.value;
    }

    explicit constexpr operator bool() const {
        return value;
    }

    constexpr void unset(map_prots flags) {
        value &= int(~flags.value);
    }

    constexpr std::string to_string() const {
        constexpr core::array flags = {
            core::tuple{map_prot::none, std::string_view("none")},
            core::tuple{map_prot::read, std::string_view("read")},
            core::tuple{map_prot::write, std::string_view("write")},
            core::tuple{map_prot::exec, std::string_view("exec")},
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

inline constexpr map_prots operator|(map_prot lhs, map_prot rhs) {
    return map_prot(int(lhs) | int(rhs));
}

/* [[[codegen end]]] */
} // namespace sys
