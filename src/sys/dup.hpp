#pragma once

#include <sys/syscall.hpp>

namespace sys {
inline auto dup(fd_t fd) {
    return syscall<fd_t>(SYS_dup, fd);
}

inline auto dup2(fd_t old, fd_t new_fd) {
    syscall_result<fd_t> res;
    do {
        res = syscall<fd_t>(SYS_dup2, old, new_fd);
    } while (res.error() == errc::ebusy);

    return res;
}
} // namespace sys
