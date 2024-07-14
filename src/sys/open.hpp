#pragma once

#include <sys/open_flags.hpp>
#include <sys/syscall.hpp>

namespace sys
{
inline auto openat(fd_t dirfd, const char* pathname, openflags oflags, file_perms mode = file_perms::none) {
    oflags |= openflag::large;
    return syscall<fd_t, trap_async_cancel>(SYS_openat, dirfd, pathname, oflags.value, mode.to_int());
}

inline auto openat(fd_t dirfd, const std::string& pathname, openflags oflags, file_perms mode = file_perms::none) {
    return openat(dirfd, pathname.data(), oflags, mode);
}

inline auto open(const char* pathname, openflags oflags, file_perms mode = file_perms::none) {
    return openat(fdcwd, pathname, oflags, mode);
}

inline auto open(const std::string& pathname, openflags oflags, file_perms mode = file_perms::none) {
    return open(pathname.data(), oflags, mode);
}
} // namespace sys
