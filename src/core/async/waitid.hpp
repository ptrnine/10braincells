#pragma once

#include <core/async/ctx.hpp>
#include <core/coro/task.hpp>
#include <sys/waitid.hpp>

namespace core::coro {
task<sys::syscall_result<sys::siginfo_t>> waitid(sys::wait_type type, sys::fd_t id, sys::wait_flags options) {
    sys::siginfo_t siginfo{};

    auto res = co_await make_awaitable<int>([&siginfo, &type, &id, &options](awaitable_base<int>& awaitable) {
        auto& sqe = current_ctx->get_sqe();
        io_uring_prep_waitid(&sqe, idtype_t(type), id_t(id), (siginfo_t*)&siginfo, options.value, 0);
        io_uring_sqe_set_data(&sqe, &awaitable);
        io_uring_submit(current_ctx->get_ring());
    });

    using result_t = sys::syscall_result<sys::siginfo_t>;
    if (!res) {
        co_return result_t::make_value(siginfo);
    }
    co_return result_t::make_error(-res);
}
} // namespace core::core
