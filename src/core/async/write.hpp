#pragma once

#include <core/async/ctx.hpp>
#include <core/concepts/trivial_span_like.hpp>
#include <core/coro/task.hpp>
#include <sys/syscall.hpp>

namespace core::async {
template <typename Lazy>
struct write_provider {
    task<sys::syscall_result<size_t>> write(sys::fd_t fd, const void* data, size_t size) {
        auto res = co_await io::uring::make_uring_awaitable(
            [&fd, &data, &size](io::uring::uring_awaitable& awaitable) {
                auto& sqe = current_ctx->get_sqe();
                io_uring_prep_write(&sqe, int(fd), data, unsigned(size), 0);
                io_uring_sqe_set_data(&sqe, &awaitable);
                io_uring_submit(current_ctx->get_ring());
            },
            async_task_type::write
        );
        co_return sys::syscall_result<size_t>{res};
    }

    /* Return ENAVAIL if write returns size less then sizeof(data) */
    task<sys::syscall_result<size_t>> write(sys::fd_t fd, const core::trivial auto& data) {
        auto res = co_await write(fd, &data, sizeof(data));
        if (res && *res < sizeof(data))
            res.set(errc{ENAVAIL});
        co_return res;
    }

    /* Return ENAVAIL if write returns size less then sizeof(data) */
    task<sys::syscall_result<size_t>> write(sys::fd_t fd, const core::trivial_span_like auto& data) {
        auto res = co_await write(fd, data.data(), data.size() * sizeof(*data.data()));
        if (res) {
            if (*res < data.size() * sizeof(*data.data()))
                res.set(errc{ENAVAIL});
            else
                *res /= sizeof(*data.data());
        }
        co_return res;
    }
};

template <typename T>
auto write(sys::fd_t fd, const T* data, size_t size) {
    return write_provider<void>{}.write(fd, data, size);
}

auto write(sys::fd_t fd, const core::trivial auto& data) {
    return write_provider<void>{}.write(fd, data);
}

auto write(sys::fd_t fd, const core::trivial_span_like auto& data) {
    return write_provider<void>{}.write(fd, data);
}
} // namespace core::async
