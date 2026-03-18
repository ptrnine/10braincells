#pragma once

#include <core/coro/task.hpp>
#include <core/io/uring/ctx.hpp>

#include <sys/waitid.hpp>

namespace core::async {
task<sys::syscall_result<sys::siginfo_t>> waitid(sys::wait_type type, sys::fd_t id, sys::wait_flags options) {
    sys::siginfo_t siginfo{};

    auto res = co_await io::uring::make_uring_awaitable(
        [&siginfo, &type, &id, &options](io::uring::uring_awaitable& awaitable) {
            auto& sqe = io::uring::current_ctx->get_sqe();
            io_uring_prep_waitid(&sqe, idtype_t(type), id_t(id), (siginfo_t*)&siginfo, options.value, 0);
            io_uring_sqe_set_data(&sqe, &awaitable);
            io_uring_submit(io::uring::current_ctx->get_ring());
        },
        async_task_type::waitid
    );

    using result_t = sys::syscall_result<sys::siginfo_t>;
    if (!res) {
        co_return result_t::make_value(siginfo);
    }
    co_return result_t::make_error(int(-res));
}
} // namespace core::async
