#pragma once

#include <core/async/ctx.hpp>
#include <core/coro/task.hpp>
#include <sys/syscall.hpp>

namespace core::coro {
template <typename Lazy = void>
task<sys::syscall_result<void>> close(sys::fd_t fd) {
    auto res = co_await make_awaitable<long>([&fd](awaitable_base<long>& awaitable) {
        auto& sqe = current_ctx->get_sqe();
        io_uring_prep_close(&sqe, int(fd));
        io_uring_sqe_set_data(&sqe, &awaitable);
        io_uring_submit(current_ctx->get_ring());
    });
    co_return sys::syscall_result<void>{res};
}
} // namespace core::coro
