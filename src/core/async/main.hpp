#pragma once

#include <core/async/ctx.hpp>
#include <core/coro/task.hpp>
#include <core/io/uring/ctx.hpp>

core::task<int> coro_main(std::span<char*> args);

namespace core {
struct coro_main_task {
    struct promise_type {
        std::suspend_never initial_suspend() noexcept {
            return {};
        }
        std::suspend_never final_suspend() noexcept {
            return {};
        }
        void           unhandled_exception() noexcept {}
        coro_main_task get_return_object() {
            return {};
        }
        void return_void() {}
    };
};

coro_main_task main_entry(int argc, char** argv) {
    current_ctx->exit(co_await coro_main(std::span{argv, size_t(argc)}));
}
} // namespace core

int main(int argc, char** argv) {
    using namespace core;

    io::uring::ctx ctx{32};
    current_ctx = &ctx;
    main_entry(argc, argv);
    return ctx.run();
}
