#pragma once

#include <core/async/ctx.hpp>
#include <core/concepts/string.hpp>
#include <core/coro/task.hpp>
#include <sys/open_flags.hpp>
#include <sys/syscall.hpp>

#define fwd(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace core::coro {
template <typename... Ts>
    requires(core::null_term_string<core::decay<Ts>> || ... || false) &&
    (core::convertible_to<Ts, sys::openflags> || ... || false)
task<sys::syscall_result<sys::fd_t>> openat(sys::fd_t dirfd, Ts&&... args) {
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

    auto res = co_await make_awaitable<long>([&dirfd, pathname, &flags, &mode](awaitable_base<long>& awaitable) {
        auto& sqe = current_ctx->get_sqe();
        io_uring_prep_openat(&sqe, int(dirfd), pathname, flags.value, mode.to_int());
        io_uring_sqe_set_data(&sqe, &awaitable);
        io_uring_submit(current_ctx->get_ring());
    });

    co_return sys::syscall_result<sys::fd_t>{res};
}

task<sys::syscall_result<sys::fd_t>> open(auto&&... args) {
    return coro::openat(sys::fdcwd, fwd(args)...);
}
} // namespace core::coro

#undef fwd
