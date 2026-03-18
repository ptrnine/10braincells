#pragma once

#include <core/concepts/string.hpp>
#include <core/coro/task.hpp>
#include <core/io/uring/ctx.hpp>

#include <sys/open_flags.hpp>
#include <sys/syscall.hpp>

#define fwd(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace core::async {
template <typename... Ts>
    requires(core::null_term_string<core::decay<Ts>> || ... || false) && (core::convertible_to<Ts, sys::openflags> || ... || false)
task<sys::syscall_result<sys::fd_t>> openat(sys::fd_t dirfd, Ts&&... args) {
    if (io::uring::current_ctx->is_tasks_blocked()) {
        co_return {errc::ecanceled};
    }

    const char*     pathname;
    sys::openflags  flags = sys::openflag::large;
    sys::file_perms mode  = sys::file_perms::o644;

    core::fwd_as_tuple(fwd(args)...)
        .foreach (
            core::overloaded{
                [&](const std::string& v) { pathname = v.data(); },
                [&](const char* v) { pathname = v; },
                [&](sys::openflags v) { flags |= v; },
                [&](sys::file_perms v) { mode |= v; },
            }
        );

    auto res = co_await io::uring::make_uring_awaitable(
        [&dirfd, pathname, &flags, &mode](io::uring::uring_awaitable& awaitable) {
            auto& sqe = io::uring::current_ctx->get_sqe();
            io_uring_prep_openat(&sqe, int(dirfd), pathname, flags.value, mode.to_int());
            io_uring_sqe_set_data(&sqe, &awaitable);
            io_uring_submit(io::uring::current_ctx->get_ring());
        },
        async_task_type::inotify_watch_wait
    );

    co_return sys::syscall_result<sys::fd_t>{res};
}

task<sys::syscall_result<sys::fd_t>> open(auto&&... args) {
    return async::openat(sys::fdcwd, fwd(args)...);
}
} // namespace core::async

#undef fwd
