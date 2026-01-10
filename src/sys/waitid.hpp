#pragma once

#include <sys/siginfo.hpp>
#include <sys/syscall.hpp>
#include <sys/wait_flags.hpp>

namespace sys {
// NOLINTNEXTLINE(performance-enum-size)
enum class wait_type {
    all   = 0,
    pid   = 1,
    pgid  = 2,
    pidfd = 3,
};

auto waitid(wait_type type, sys::fd_t id, wait_flags flags) {
    syscall_result<siginfo_t> info{sys::type<siginfo_t>};

    auto res = syscall<void>(SYS_waitid, type, id, &info.unsafe_get(), flags.value, 0);
    if (!res)
        info.set(res.unsafe_error());
    return info;
}
} // namespace sys
