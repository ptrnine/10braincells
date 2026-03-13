#pragma once

#include <future>

#include <core/async/ctx.hpp>
#include <core/coro/async_generator.hpp>
#include <core/coro/task.hpp>
#include <sys/eventfd.hpp>
#include <sys/poll.hpp>
#include <sys/readdir.hpp>
#include <sys/write.hpp>

namespace core::async {
template <size_t BuffSize = sys::dirent_default_buffer_size>
task<sys::syscall_result<sys::dirent_result<u8[BuffSize]>>> getdents(sys::fd_t fd) {
    std::future<sys::syscall_result<sys::dirent_result<u8[BuffSize]>>> res;

    co_await make_awaitable<long>([&res, fd](io::uring::uring_awaitable& awaitable) mutable {
        current_ctx->schedule_thread_task(awaitable, [&res, fd](sys::fd_t efd) mutable {
            res = std::async(std::launch::async, [fd]{
                return sys::getdents<BuffSize>(fd);
            });
            res.wait();
            sys::write(efd, u64(1));
        });
    });

    co_return res.get();
}

task<sys::syscall_result<sys::dirent_result<u8*>>> getdents(sys::fd_t fd, std::span<u8> buff) {
    std::future<sys::syscall_result<sys::dirent_result<u8*>>> res;

    co_await make_awaitable<long>([&res, fd, buff](io::uring::uring_awaitable& awaitable) mutable {
        current_ctx->schedule_thread_task(awaitable, [&res, fd, buff](sys::fd_t efd) mutable {
            res = std::async(std::launch::async, [fd, buff]{
                return sys::getdents(fd, buff);
            });
            res.wait();
            sys::write(efd, u64(1));
        });
    });

    co_return res.get();
}

template <size_t BuffSize = sys::dirent_default_buffer_size, typename Fd = sys::fd_t>
async_generator<sys::dir_entry_str> readdir(Fd fd) {
    while (true) {
        auto res = (co_await async::getdents<BuffSize>(fd)).get();
        if (res.is_end()) {
            co_return;
        }

        while (!res.is_end()) {
            auto value = res.get().to_dir_entry_str();
            res.next();
            co_yield value;
        }
    }
}
} // namespace core::async
