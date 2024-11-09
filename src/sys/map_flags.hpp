#pragma once

#include <string>

#include <core/array.hpp>
#include <core/tuple.hpp>

namespace sys {
/*
 * [[[codegen start]]]
 * Generated with:
 * ./build-deb/codegen/flags map_flag map_flags int shared=0x01 priv=0x02 type=0x0f fixed=0x10 anon=0x20 noreserve=0x4000 growsdown=0x0100 denywrite=0x0800 executable=0x1000 locked=0x2000 populate=0x8000 nonblock=0x10000 stack=0x20000 hugetlb=0x40000 sync=0x80000 fixed_noreplace=0x100000
 */

enum class map_flag : int {
    shared          = 0x01,
    priv            = 0x02,
    type            = 0x0f,
    fixed           = 0x10,
    anon            = 0x20,
    noreserve       = 0x4000,
    growsdown       = 0x0100,
    denywrite       = 0x0800,
    executable      = 0x1000,
    locked          = 0x2000,
    populate        = 0x8000,
    nonblock        = 0x10000,
    stack           = 0x20000,
    hugetlb         = 0x40000,
    sync            = 0x80000,
    fixed_noreplace = 0x100000,
};

struct map_flags {
    using enum map_flag;

    constexpr map_flags() = default;
    constexpr map_flags(map_flag ivalue): value(int(ivalue)) {}

    constexpr map_flags operator|(map_flags flag) const {
        return map_flag(int(value | flag.value));
    }

    constexpr map_flags operator&(map_flags flag) const {
        return map_flag(int(value & flag.value));
    }

    constexpr map_flags& operator|=(map_flags flag) {
        value = int(value | flag.value);
        return *this;
    }

    constexpr bool test(map_flags flags) const {
        return value & flags.value;
    }

    explicit constexpr operator bool() const {
        return value;
    }

    constexpr void unset(map_flags flags) {
        value &= int(~flags.value);
    }

    constexpr std::string to_string() const {
        constexpr core::array flags = {
            core::tuple{map_flag::shared, std::string_view("shared")},
            core::tuple{map_flag::priv, std::string_view("priv")},
            core::tuple{map_flag::type, std::string_view("type")},
            core::tuple{map_flag::fixed, std::string_view("fixed")},
            core::tuple{map_flag::anon, std::string_view("anon")},
            core::tuple{map_flag::noreserve, std::string_view("noreserve")},
            core::tuple{map_flag::growsdown, std::string_view("growsdown")},
            core::tuple{map_flag::denywrite, std::string_view("denywrite")},
            core::tuple{map_flag::executable, std::string_view("executable")},
            core::tuple{map_flag::locked, std::string_view("locked")},
            core::tuple{map_flag::populate, std::string_view("populate")},
            core::tuple{map_flag::nonblock, std::string_view("nonblock")},
            core::tuple{map_flag::stack, std::string_view("stack")},
            core::tuple{map_flag::hugetlb, std::string_view("hugetlb")},
            core::tuple{map_flag::sync, std::string_view("sync")},
            core::tuple{map_flag::fixed_noreplace, std::string_view("fixed_noreplace")},
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

inline constexpr map_flags operator|(map_flag lhs, map_flag rhs) {
    return map_flag(int(lhs) | int(rhs));
}

/* [[[codegen end]]] */
} // namespace sys
