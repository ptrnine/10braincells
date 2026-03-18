/* Command: ./build/codegen/cg_flags name=inotify_flag name_combined=inotify_flags type=int namespace_name=sys nonblock=IN_NONBLOCK close_exec=IN_CLOEXEC */
#pragma once

#include <string>

#include <core/array.hpp>
#include <core/tuple.hpp>

#include <sys/inotify.h>

namespace sys {
enum class inotify_flag : int {
    nonblock   = 00004000,
    close_exec = 02000000,
};

struct inotify_flags {
    using enum inotify_flag;

    constexpr inotify_flags() = default;
    constexpr inotify_flags(inotify_flag ivalue): value(int(ivalue)) {}

    constexpr inotify_flags operator|(inotify_flags flag) const {
        return inotify_flag(int(value | flag.value));
    }

    constexpr inotify_flags operator&(inotify_flags flag) const {
        return inotify_flag(int(value & flag.value));
    }

    constexpr inotify_flags& operator|=(inotify_flags flag) {
        value = int(value | flag.value);
        return *this;
    }

    constexpr bool test(inotify_flags flags) const {
        return value & flags.value;
    }

    explicit constexpr operator bool() const {
        return value;
    }

    constexpr void unset(inotify_flags flags) {
        value &= int(~flags.value);
    }

    constexpr std::string to_string() const {
        constexpr core::array flags = {
            core::tuple{inotify_flag::nonblock, std::string_view("nonblock")},
            core::tuple{inotify_flag::close_exec, std::string_view("close_exec")},
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

inline constexpr inotify_flags operator|(inotify_flag lhs, inotify_flag rhs) {
    return inotify_flag(int(lhs) | int(rhs));
}

inline static constexpr wd_t invalid_wd{-1};
} // namespace sys
