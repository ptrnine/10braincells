#pragma once

#include <sys/pidfd_open_flags.hpp>
#include <sys/syscall.hpp>

namespace sys {
inline auto pidfd_open(sys::pid_t pid, pidfd_open_flags flags = {}) {
    return syscall<fd_t>(SYS_pidfd_open, pid, flags.value);
}
} // namespace sys
