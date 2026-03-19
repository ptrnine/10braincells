#pragma once

#include <exception>
#include <future>

#include <core/async/cancel.hpp>
#include <core/async/inotify_ctx.hpp>
#include <core/coro/task.hpp>
#include <core/coro/wait_all.hpp>
#include <core/finalizer.hpp>
#include <core/io/uring/ctx.hpp>

#include <core/io/file.hpp>

#include <sys/open_flags.hpp>
#include <sys/signalfd.hpp>
#include <sys/sigprocmask.hpp>
#include <sys/siginfo.hpp>
#include <thread>

#define fwd(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace core::async {
template <typename Task, typename ResultT>
struct runner_promise {
    std::suspend_never initial_suspend() noexcept {
        return {};
    }
    std::suspend_always final_suspend() noexcept {
        return {};
    }
    Task get_return_object() {
        return Task{std::coroutine_handle<runner_promise>::from_promise(*this)};
    }
    void unhandled_exception() noexcept {
        _exception = std::current_exception();
    }
    void return_value(ResultT value) noexcept {
        _result = mov(value);
    }

    auto result() {
        if (_exception) {
            std::rethrow_exception(_exception);
        }
        return mov(_result);
    }

    async::cancelation_point_t _cancelation_point;
    ResultT                    _result;
    std::exception_ptr         _exception;

#ifdef CORO_METAINFO
    coro_handle_metainfo _metainfo;
#endif

    void set_metainfo([[maybe_unused]] coro_handle_metainfo metainfo) {
#ifdef CORO_METAINFO
        _metainfo = metainfo;
#endif
    }
};

template <typename Task>
struct runner_promise<Task, void> {
    std::suspend_never initial_suspend() noexcept {
        return {};
    }
    std::suspend_always final_suspend() noexcept {
        return {};
    }
    Task get_return_object() {
        return Task{std::coroutine_handle<runner_promise>::from_promise(*this)};
    }
    void unhandled_exception() noexcept {
        _exception = std::current_exception();
    }
    void return_void() noexcept {}

    void result() {
        if (_exception) {
            std::rethrow_exception(_exception);
        }
    }

    async::cancelation_point_t _cancelation_point;
    std::exception_ptr         _exception;

#ifdef CORO_METAINFO
    coro_handle_metainfo _metainfo;
#endif

    void set_metainfo([[maybe_unused]] coro_handle_metainfo metainfo) {
#ifdef CORO_METAINFO
        _metainfo = metainfo;
#endif
    }
};

template <typename ResultT>
struct runner_task {
    ~runner_task() {
        if (_handle) {
            _handle.destroy();
        }
    }

    using promise_type = runner_promise<runner_task, ResultT>;
    std::coroutine_handle<promise_type> _handle;
};

namespace details {
    task<void> stop_by_signal(sys::fd_t sigfd) {
        if (sigfd == sys::invalid_fd) {
            co_return;
        }

        while (auto siginfo = co_await async::read<sys::siginfo_t>(sigfd)) {
            if (siginfo->signo == SIGINT || siginfo->signo == SIGTERM) {
                for (auto sigpipe : io::uring::current_ctx->child_signalfd_pipes()) {
                    sys::write(sigpipe.out, *siginfo);
                }

                io::uring::current_ctx->send_kill_event(siginfo->signo);

                glog().warn("Received signal {} in thread {x}, terminating", siginfo->signo == SIGINT ? "SIGINT" : "SIGTERM", std::this_thread::get_id());
                // if (current_ctx->child_signalfd_pipes().size())
                (co_await async::cancel_all()).throw_if_error();
                io::uring::current_ctx->block_new_tasks();
                break;
            }
        }
    }

    task<void> stop_by_kill_event() {
        auto eventfd = io::uring::current_ctx->get_internal_kill_event();
        if (eventfd == sys::invalid_fd) {
            co_return;
        }

        if (auto signo = co_await async::read<u64>(eventfd)) {
            // XXX: siginfo does not contain full info, only signo
            sys::siginfo_t siginfo{};
            siginfo.signo = int(signo.get());
            for (auto sigpipe : io::uring::current_ctx->child_signalfd_pipes()) {
                sys::write(sigpipe.out, siginfo);
            }

            io::uring::current_ctx->send_kill_event(int(*signo));

            glog().warn("Received kill event {} in thread {x}, terminating", *signo == SIGINT ? "SIGINT" : "SIGTERM", std::this_thread::get_id());
            // if (current_ctx->child_signalfd_pipes().size())
            (co_await async::cancel_all()).throw_if_error();
            io::uring::current_ctx->block_new_tasks();
        }
    }

    auto runner_launch(auto&& start_coro, sys::fd_t signalfd) -> task<typename decay<decltype(start_coro())>::result_type> {
        auto main       = start_coro();
        auto inotify    = async::current_inotify_ctx->run();
        auto signal     = stop_by_signal(signalfd);
        auto kill_event = stop_by_kill_event();

        std::exception_ptr exception;

        if constexpr (is_same<void, typename decay<decltype(start_coro())>::result_type>) {
            try {
                co_await main;
            } catch (...) {
                exception = std::current_exception();
            }

            co_await async::current_inotify_ctx->stop();
            co_await inotify;
            io::uring::current_ctx->block_new_tasks();

            co_await async::wait_all(signal.cancel(), kill_event.cancel(), signal, kill_event, current_final_task_waiter->wait_all());

            if (exception) {
                std::rethrow_exception(exception);
            }
        } else {
            opt<typename decay<decltype(start_coro())>::result_type> res;
            try {
                res = co_await main;
            } catch (...) {
                exception = std::current_exception();
            }

            co_await async::current_inotify_ctx->stop();
            co_await inotify;
            io::uring::current_ctx->block_new_tasks();

            co_await async::wait_all(signal.cancel(), kill_event.cancel(), signal, kill_event, current_final_task_waiter->wait_all());

            if (exception) {
                std::rethrow_exception(exception);
            }

            co_return *res;
        }
    }

    auto runner_coro_entry(auto&& start_coro, sys::fd_t signalfd) -> runner_task<typename decay<decltype(start_coro())>::result_type> {
        finalizer on_exit{[] { io::uring::current_ctx->exit(); }};
        co_return co_await runner_launch(fwd(start_coro), signalfd);
    }
} // namespace details

thread_local io::file* current_signalfd = nullptr;

auto run_io_ctx(auto&& start_coro) {
    glog().info("start async context at thread {x}", std::this_thread::get_id());

    auto      prev_ctx = std::tuple{io::uring::current_ctx, async::current_inotify_ctx, current_final_task_waiter};
    finalizer on_exit{[&] { std::tie(io::uring::current_ctx, async::current_inotify_ctx, current_final_task_waiter) = prev_ctx; }};

    io::uring::ctx ctx{32, io::uring::setup_flags::single_issuer};
    io::uring::current_ctx = &ctx;

    if (std::get<0>(prev_ctx)) {
        io::uring::current_ctx->set_kill_event_recipient(std::get<0>(prev_ctx)->ensure_internal_kill_event());
    }

    async::inotify_ctx inotify_ctx;
    async::current_inotify_ctx = &inotify_ctx;

    final_task_waiter waiter;
    current_final_task_waiter = &waiter;

    auto result = async::details::runner_coro_entry(fwd(start_coro), *current_signalfd);
    ctx.run();
    return result._handle.promise().result();
}

auto async_run_io_ctx(auto&& start_coro) -> task<typename decay<decltype(start_coro())>::result_type> {
    std::future<typename decay<decltype(start_coro())>::result_type> res;

    auto sigpipe = io::file::pipe();

    co_await make_awaitable<long>(
        [&res, coro = fwd(start_coro), &sigpipe]<typename Promise>(io::uring::uring_awaitable& awaitable, std::coroutine_handle<Promise>& caller) mutable {
            if (io::uring::current_ctx->is_tasks_blocked()) {
                throw errc_exception{errc::ecanceled};
            }

            caller.promise()._cancelation_point.set((u64)&awaitable, awaitable_type::uring_threaded);
            caller.promise().set_metainfo({awaitable_type::uring_threaded, async_task_type::spawn_child});

            io::uring::current_ctx->add_child_signalfd_pipe(sigpipe);
            io::uring::current_ctx->schedule_thread_task(awaitable, [&res, coro = mov(coro), sigfd = &sigpipe.in](sys::fd_t efd) mutable {
                res = std::async(std::launch::async, [coro = mov(coro), efd, sigfd] mutable {
                    finalizer f{[efd] { sys::write(efd, u64(1)); }};
                    current_signalfd = sigfd;
                    return run_io_ctx(mov(coro));
                });
            });
        }
    );
    io::uring::current_ctx->remove_child_signalfd_pipe(sigpipe);

    co_return res.get();
}
} // namespace core::async

#undef fwd
