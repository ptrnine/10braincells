#pragma once

#include <chrono>

#include <core/async/ctx.hpp>
#include <core/coro/task.hpp>
#include <sys/syscall.hpp>

namespace core::coro {
template <typename Rep, typename Period>
__kernel_timespec to_kernel_timespec(const std::chrono::duration<Rep, Period>& d) {
    using namespace std::chrono;

    auto ns = duration_cast<nanoseconds>(d);

    int64_t   sec  = duration_cast<seconds>(ns).count();
    long long nsec = (ns - seconds(sec)).count();

    return __kernel_timespec{.tv_sec = sec, .tv_nsec = nsec};
}

task<sys::syscall_result<void>> sleep(std::chrono::nanoseconds duration) {
    auto dur = to_kernel_timespec(duration);

    auto res = co_await make_awaitable<int>([&dur](awaitable_base<int>& awaitable) {
        auto& sqe = current_ctx->get_sqe();
        io_uring_prep_timeout(&sqe, &dur, 0, 0);
        io_uring_sqe_set_data(&sqe, &awaitable);
        io_uring_submit(current_ctx->get_ring());
    });

    co_return sys::syscall_result<void>::make_error({-res});
}
} // namespace core::core
