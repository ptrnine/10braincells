#pragma once

#include <core/async/inotify_ctx.hpp>
#include <core/coro/task.hpp>
#include <core/io/uring/ctx.hpp>

namespace core::async {
task<sys::syscall_result<size_t>> cancel(cancelation_point_t cancelation_point) {
    auto type = cancelation_point.type();

    //glog().debug("cancel {x}", cancelation_point.get());

    if (type == awaitable_type::uring) {
        auto wait_res = co_await io::uring::make_uring_awaitable(
            [cancelation_point](io::uring::uring_awaitable& awaitable) {
                auto& sqe = io::uring::current_ctx->get_sqe();
                io_uring_prep_cancel64(&sqe, cancelation_point.get(), 0);
                io_uring_sqe_set_data(&sqe, &awaitable);
                io_uring_submit(io::uring::current_ctx->get_ring());
            },
            async_task_type::cancel
        );
        co_return sys::syscall_result<size_t>{wait_res};
    } else if (type == awaitable_type::uring_threaded) {
        // TODO: threaded task cancelation
        // co_return {0};
    } else if (type == awaitable_type::inotify_wd_event) {
        co_return current_inotify_ctx->cancel(cancelation_point.awaitable());
    }

    throw std::runtime_error("Unknown cancelation point type: " + std::to_string(int(type)));
}

task<sys::syscall_result<size_t>> cancel_all() {
    auto wait_res = co_await io::uring::make_uring_awaitable(
        [](io::uring::uring_awaitable& awaitable) {
            auto& sqe = io::uring::current_ctx->get_sqe();
            io_uring_prep_cancel(&sqe, 0, IORING_ASYNC_CANCEL_ANY | IORING_ASYNC_CANCEL_ALL);
            io_uring_sqe_set_data(&sqe, &awaitable);
            io_uring_submit(io::uring::current_ctx->get_ring());
        },
        async_task_type::cancel
    );
    co_return sys::syscall_result<size_t>{wait_res};
}
} // namespace core::async

namespace core {
template <typename ResultT>
task<sys::syscall_result<void>> task<ResultT>::cancel() {
    if (auto cp = cancelation_point()) {
        auto res = co_await async::cancel(cp);
        if (!res) {
            co_return sys::syscall_result<void>::make_error(res.unsafe_error());
        }
    }
    co_return sys::syscall_result<void>{};
}
} // namespace core
