/* Command: ./build/codegen/cg_flags name=eventfd_flag name_combined=eventfd_flags type=int close_exec=EFD_CLOEXEC nonblock=EFD_NONBLOCK semaphore=EFD_SEMAPHORE */
#pragma once

#include <string>

#include <core/array.hpp>
#include <core/tuple.hpp>
#include <sys/eventfd.h>

namespace sys {
enum class eventfd_flag : int {
    semaphore  = 00000001,
    nonblock   = 00004000,
    close_exec = 02000000,
};

struct eventfd_flags {
    using enum eventfd_flag;

    constexpr eventfd_flags() = default;
    constexpr eventfd_flags(eventfd_flag ivalue): value(int(ivalue)) {}

    constexpr eventfd_flags operator|(eventfd_flags flag) const {
        return eventfd_flag(int(value | flag.value));
    }

    constexpr eventfd_flags operator&(eventfd_flags flag) const {
        return eventfd_flag(int(value & flag.value));
    }

    constexpr eventfd_flags& operator|=(eventfd_flags flag) {
        value = int(value | flag.value);
        return *this;
    }

    constexpr bool test(eventfd_flags flags) const {
        return value & flags.value;
    }

    explicit constexpr operator bool() const {
        return value;
    }

    constexpr void unset(eventfd_flags flags) {
        value &= int(~flags.value);
    }

    constexpr std::string to_string() const {
        constexpr core::array flags = {
            core::tuple{eventfd_flag::close_exec, std::string_view("close_exec")},
            core::tuple{eventfd_flag::nonblock, std::string_view("nonblock")},
            core::tuple{eventfd_flag::semaphore, std::string_view("semaphore")},
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

inline constexpr eventfd_flags operator|(eventfd_flag lhs, eventfd_flag rhs) {
    return eventfd_flag(int(lhs) | int(rhs));
}
} // namespace sys
