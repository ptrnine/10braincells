#pragma once

#include <sys/mman.h>

#include <core/array.hpp>
#include <core/tuple.hpp>
#include <string>

/*
 * [[[codegen start]]]
 * Generated with:
 * ./build-rel/codegen/flags memfd_flag memfd_flags int cloexec=MFD_CLOEXEC allow_sealing=MFD_ALLOW_SEALING
 * hugetlb=MFD_HUGETLB
 */

namespace sys {
enum class memfd_flag : int {
    cloexec       = MFD_CLOEXEC,
    allow_sealing = MFD_ALLOW_SEALING,
    hugetlb       = MFD_HUGETLB,
};

struct memfd_flags {
    using enum memfd_flag;

    constexpr memfd_flags() = default;
    constexpr memfd_flags(memfd_flag ivalue): value(int(ivalue)) {}

    constexpr memfd_flags operator|(memfd_flags flag) const {
        return memfd_flag(value | flag.value);
    }

    constexpr memfd_flags operator&(memfd_flags flag) const {
        return memfd_flag(value & flag.value);
    }

    constexpr memfd_flags& operator|=(memfd_flags flag) {
        value = int(value | flag.value);
        return *this;
    }

    constexpr memfd_flags& operator&=(memfd_flags flag) {
        value = int(value & flag.value);
        return *this;
    }

    constexpr bool test(memfd_flags flags) const {
        return value & flags.value;
    }

    explicit constexpr operator bool() const {
        return value;
    }

    constexpr std::string to_string() const {
        constexpr core::array flags = {
            core::tuple{memfd_flag::cloexec, std::string_view("cloexec")},
            core::tuple{memfd_flag::allow_sealing, std::string_view("allow_sealing")},
            core::tuple{memfd_flag::hugetlb, std::string_view("hugetlb")},
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

inline constexpr memfd_flags operator|(memfd_flag lhs, memfd_flag rhs) {
    return memfd_flag(int(lhs) | int(rhs));
}
} // namespace sys

/* [[[codegen end]]] */
