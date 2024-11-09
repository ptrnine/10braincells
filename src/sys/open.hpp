#pragma once

#include <core/concepts/string.hpp>

#include <sys/open_flags.hpp>
#include <sys/syscall.hpp>

#define fwd(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace sys
{
template <typename... Ts>
    requires(core::null_term_string<core::remove_cvref<Ts>> || ... || false) &&
            (core::convertible_to<Ts, openflags> || ... || false)
inline auto openat(fd_t dirfd, Ts&&... args) {
    const char* pathname;
    openflags   flags = openflag::large;
    file_perms  mode  = file_perms::none;

    core::fwd_as_tuple(fwd(args)...)
        .foreach (core::overloaded{
            [&](const std::string& v) {
                pathname = v.data();
            },
            [&](const char* v) {
                pathname = v;
            },
            [&](openflags v) { flags |= v; },
            [&](file_perms v) { mode |= v; },
        });

    return syscall<fd_t, trap_async_cancel>(SYS_openat, dirfd, pathname, flags.value, mode.to_int());
}

inline auto open(auto&&... args) {
    return openat(fdcwd, fwd(args)...);
}
} // namespace sys

#undef fwd
