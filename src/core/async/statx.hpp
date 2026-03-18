#pragma once

#include <core/coro/task.hpp>
#include <core/io/uring/ctx.hpp>

#include <sys/statx_types.hpp>

#define fwd(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace core::async {
task<sys::syscall_result<sys::statx_info>> statx(auto&&... args) {
    if (io::uring::current_ctx->is_tasks_blocked()) {
        co_return {errc::ecanceled};
    }

    sys::fd_t        dirfd    = sys::fdcwd;
    const char*      pathname = "";
    sys::statx_flags flags    = sys::statx_flag::empty_path;
    sys::statx_masks mask     = sys::statx_mask::all;

    core::fwd_as_tuple(fwd(args)...)
        .foreach (
            core::overloaded{
                [&](sys::fd_t v) {
                    dirfd = v;
                    if (v == sys::fdcwd)
                        flags.unset(sys::statx_flag::empty_path);
                },
                [&](sys::statx_flags v) { flags = v; },
                [&](sys::statx_masks v) { mask = v; },
                [&](const char* v) {
                    pathname = v;
                    flags.unset(sys::statx_flag::empty_path);
                },
                [&](const std::string& v) {
                    pathname = v.data();
                    flags.unset(sys::statx_flag::empty_path);
                },
            }
        );

    sys::syscall_result<sys::statx_info> info{type<sys::statx_info>};
    auto                                 res = co_await io::uring::make_uring_awaitable(
        [dirfd, pathname, flags, mask, buff = &info.unsafe_get()](io::uring::uring_awaitable& awaitable) {
            auto& sqe = io::uring::current_ctx->get_sqe();
            io_uring_prep_statx(&sqe, int(dirfd), pathname, int(flags.value), mask.value, (struct statx*)buff);
            io_uring_sqe_set_data(&sqe, &awaitable);
            io_uring_submit(io::uring::current_ctx->get_ring());
        },
        async_task_type::statx
    );

    if (res < 0) {
        info.set({-res});
    }
    co_return info;
}
} // namespace core::async
