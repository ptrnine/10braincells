#pragma once

#include <core/async/task.hpp>

namespace core::async {

template <typename T>
struct ignore_result {
    task<T> task;

    bool await_ready() noexcept {
        return task.await_ready();
    }

    template <typename Promise>
    void await_suspend(std::coroutine_handle<Promise> handle) noexcept {
        task.await_suspend(handle);
    }

    void await_resume() noexcept {}
};
} // namespace core::async
