#pragma once

#include <sys/syscall.hpp>

extern "C" {
pid_t fork();
}

namespace sys {
inline auto fork() {
    auto res = ::fork();
    if (res == -1) {
        return syscall_result<sys::pid_t>::make_error(errc::from_errno());
    }
    return syscall_result<sys::pid_t>{res};
}

inline auto vfork() {
    return syscall<sys::pid_t>(SYS_vfork);
}
} // namespace sys
