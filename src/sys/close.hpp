#pragma once

#include <sys/syscall.hpp>

namespace sys
{
inline auto close(fd_t fd) {
    return syscall<void, trap_async_cancel>(SYS_close, fd);
}

inline auto close_no_cp(fd_t fd) {
    return syscall<void>(SYS_close, fd);
}

inline auto close_range(u32 start, u32 max, u32 flags = 0) {
    return syscall<void>(SYS_close_range, start, max, flags);
}
} // namespace sys
