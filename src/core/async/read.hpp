#pragma once

#include <core/async/ctx.hpp>
#include <core/concepts/trivial.hpp>
#include <core/concepts/trivial_span_like.hpp>
#include <core/coro/task.hpp>
#include <sys/syscall.hpp>

namespace core::coro {
template <typename Lazy>
struct read_provider {
    task<sys::syscall_result<size_t>> read(sys::fd_t fd, void* output, size_t size) {
        auto res = co_await make_awaitable<long>([&fd, &output, &size](awaitable_base<long>& awaitable) {
            auto& sqe = current_ctx->get_sqe();
            io_uring_prep_read(&sqe, int(fd), output, unsigned(size), 0);
            io_uring_sqe_set_data(&sqe, &awaitable);
            io_uring_submit(current_ctx->get_ring());
        });
        co_return sys::syscall_result<size_t>{res};
    }

    /* Return ENAVAIL if read() returns less than sizeof(T) */
    template <typename T> requires core::trivial<T>
    task<sys::syscall_result<T>> read(sys::fd_t fd) {
        sys::syscall_result<T> result{core::type<T>};

        auto res = co_await read(fd, &result.unsafe_get(), sizeof(T));
        if (!res)
            result.set(res.error());
        else if (*res != sizeof(T))
            result.set(errc{ENAVAIL});

        co_return result;
    }

    /* Return ENAVAIL if size returned by read() divided by sizeof(T) with remainder */
    task<sys::syscall_result<size_t>> read(sys::fd_t fd, core::trivial_span_like auto& output) {
        constexpr auto type_size = sizeof(*output.data());
        auto res = co_await read(fd, output.data(), output.size() * type_size);
        if (res) {
            auto remains = *res % type_size;
            if (remains)
                res.set(errc{ENAVAIL});
            else
                *res /= type_size;
        }
        co_return res;
    }

    /* Return ENAVAIL if size returned by read() divided by sizeof(T) with remainder */
    template <typename T, size_t S> requires core::trivial<T>
    task<sys::syscall_result<size_t>> read(sys::fd_t fd, T (&output)[S]) {
        constexpr auto type_size = sizeof(T);
        auto res = co_await read(fd, output, S * type_size);
        if (res) {
            auto remains = *res % type_size;
            if (remains)
                res.set(errc{ENAVAIL});
            else
                *res /= type_size;
        }
        co_return res;
    }
};

template <typename T>
auto read(sys::fd_t fd, T* output, size_t size) {
    return read_provider<void>{}.read(fd, output, size);
}

/* Return ENAVAIL if read() returns less than sizeof(T) */
template <typename T> requires core::trivial<T>
auto read(sys::fd_t fd) {
    return read_provider<void>{}.read<T>(fd);
}

/* Return ENAVAIL if size returned by read() divided by sizeof(T) with remainder */
template <typename T, size_t S> requires core::trivial<T>
auto read(sys::fd_t fd, T (&output)[S]) {
    return read_provider<void>{}.read(fd, output);
}

auto read(sys::fd_t fd, core::trivial_span_like auto& output) {
    return read_provider<void>{}.read(fd, output);
}
} // namespace core::coro
