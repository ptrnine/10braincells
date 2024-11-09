#pragma once

#include <sys/syscall.hpp>

namespace sys
{
enum class seek_whence {
    set = SEEK_SET,
    cur = SEEK_CUR,
    end = SEEK_END,
};

inline auto lseek(fd_t fd, off_t offset, seek_whence whence) {
    return syscall<off_t>(SYS_lseek, fd, offset, whence);
}
} // namespace sys
