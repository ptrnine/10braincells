#pragma once

#include <core/coro/task.hpp>
#include <core/io/uring/ctx.hpp>

#include <sys/syscall.hpp>

namespace core::async {
template <typename Lazy = void>
task<sys::syscall_result<void>> close(sys::fd_t fd) {
    if (io::uring::current_ctx->is_tasks_blocked()) {
        co_return {errc::ecanceled};
    }

    auto res = co_await io::uring::make_uring_awaitable(
        [&fd](io::uring::uring_awaitable& awaitable) {
            auto& sqe = io::uring::current_ctx->get_sqe();
            io_uring_prep_close(&sqe, int(fd));
            io_uring_sqe_set_data(&sqe, &awaitable);
            io_uring_submit(io::uring::current_ctx->get_ring());
        },
        async_task_type::close
    );
    co_return sys::syscall_result<void>{res};
}
} // namespace core::async
