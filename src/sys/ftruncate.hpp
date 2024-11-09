#pragma once

#include <sys/syscall.hpp>

namespace sys
{
[[nodiscard]]
inline auto ftruncate(sys::fd_t fd, off_t length) {
    return syscall<void>(SYS_ftruncate, fd, length);
}
} // namespace sys
