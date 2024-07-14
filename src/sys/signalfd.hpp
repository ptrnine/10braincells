#pragma once

#include <csignal>
#include <fcntl.h>
#include <string>

#include <core/array.hpp>
#include <core/bit.hpp>
#include <core/tuple.hpp>
#include <sys/basic_types.hpp>
#include <sys/signal.hpp>
#include <sys/syscall.hpp>

namespace sys {
/*
 * [[[codegen start]]]
 * Generated with:
 * ./build-deb/codegen/flags sigfd_flag sigfd_flags int close_exec=O_CLOEXEC nonblock=O_NONBLOCK
 */

enum class sigfd_flag : int {
    none       = 0,
    close_exec = O_CLOEXEC,
    nonblock   = O_NONBLOCK,
};

struct sigfd_flags {
    constexpr sigfd_flags() = default;
    constexpr sigfd_flags(sigfd_flag ivalue): value(int(ivalue)) {}

    constexpr sigfd_flags operator|(sigfd_flag flag) const {
        return sigfd_flag(value | int(flag));
    }

    constexpr sigfd_flags operator&(sigfd_flag flag) const {
        return sigfd_flag(value & int(flag));
    }

    constexpr sigfd_flags& operator|=(sigfd_flag flag) {
        value = int(value | int(flag));
        return *this;
    }

    constexpr bool test(sigfd_flags flags) const {
        return value & flags.value;
    }

    explicit constexpr operator bool() const {
        return value;
    }

    constexpr std::string to_string() const {
        constexpr core::array flags = {
            core::tuple{sigfd_flag::close_exec, std::string_view("close_exec")},
            core::tuple{sigfd_flag::nonblock, std::string_view("nonblock")},
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

inline constexpr sigfd_flags operator|(sigfd_flag lhs, sigfd_flag rhs) {
    return sigfd_flag(int(lhs) | int(rhs));
}

/* [[[codegen end]]] */


inline auto signalfd(fd_t fd, sigset mask, sigfd_flags flags = sigfd_flag::none) {
    return syscall<fd_t>(SYS_signalfd4, fd, &mask, _NSIG / 8, flags.value);
}

inline auto signalfd(sigset mask, sigfd_flags flags = sigfd_flag::none) {
    return syscall<fd_t>(SYS_signalfd4, -1, &mask, _NSIG / 8, flags.value);
}

struct siginfo {
    u32 signo;
    i32 _pad1;
    i32 code;
    u32 pid;
    u32 uid;
    i32 fd;
    u32 tid;
    u32 band;
    u32 overrun;
    u32 trapno;
    i32 status;
    i32 integer;
    u64 ptr;
    u64 utime;
    u64 stime;
    u64 addr;
    u16 addr_lsb;
    u16 _pad2;
    i32 syscall;
    u64 call_addr;
    u32 arch;
    u8  _pad[28];
};
} // namespace sys
