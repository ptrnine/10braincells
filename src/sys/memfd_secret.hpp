#pragma once

#include <sys/open_flags.hpp>
#include <sys/syscall.hpp>

namespace sys
{
inline auto memfd_secret(bool close_exec = false) {
    return syscall<fd_t>(SYS_memfd_secret, close_exec ? O_CLOEXEC : 0);
}
} // namespace sys
