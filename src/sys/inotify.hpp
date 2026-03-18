#pragma once

#include <core/concepts/string.hpp>

#include <sys/inotify_flags.hpp>
#include <sys/inotify_watch_flags.hpp>
#include <sys/syscall.hpp>

namespace sys {
struct inotify_event {
    std::string_view name() const {
        return {_name, len};
    }

    size_t size() const {
        return sizeof(inotify_event) + len;
    }

    sys::wd_t           wd;
    inotify_watch_flags mask;
    u32                 cookie;
    u32                 len;
    char                _name[];
};

auto inotify_init(inotify_flags flags = {}) {
    return syscall<fd_t>(SYS_inotify_init1, flags.value);
}

template <typename Str> requires core::null_term_string<core::decay<Str>>
auto inotify_add_watch(fd_t ino_fd, const Str& path, inotify_watch_flags flags) {
    const char* pathname;
    if constexpr (type<Str> == type<std::string>) {
        pathname = path.data();
    } else {
        pathname = path;
    }

    return syscall<wd_t>(SYS_inotify_add_watch, int(ino_fd), pathname, flags.value);
}

auto inotify_rm_watch(fd_t ino_fd, wd_t wd) {
    return syscall<void>(SYS_inotify_rm_watch, int(ino_fd), int(wd));
}
} // namespace sys
