#pragma once

#include <core/meta/type.hpp>
#include <sys/statx_types.hpp>
#include <sys/syscall.hpp>

#define fwd(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace sys
{
auto statx(auto&&... args) {
    fd_t        dirfd    = fdcwd;
    const char* pathname = "";
    statx_flags flags    = statx_flag::empty_path;
    statx_masks mask     = statx_mask::all;

    core::fwd_as_tuple(fwd(args)...)
        .foreach (core::overloaded{
            [&](fd_t v) {
                dirfd = v;
                if (v == fdcwd)
                    flags.unset(statx_flag::empty_path);
            },
            [&](statx_flags v) { flags = v; },
            [&](statx_masks v) { mask = v; },
            [&](const char* v) {
                pathname = v;
                flags.unset(statx_flag::empty_path);
            },
            [&](const std::string& v) {
                pathname = v.data();
                flags.unset(statx_flag::empty_path);
            },
        });

    syscall_result<statx_info> info{type<statx_info>};
    auto                       res = syscall<void>(SYS_statx, dirfd, pathname, flags.value, mask.value, &info);
    if (!res)
        info.set(res.unsafe_error());
    return info;
}
} // namespace sys
