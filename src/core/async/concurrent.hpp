#pragma once

#include <queue>

#include <core/async/ignore_result.hpp>
#include <util/log.hpp>

namespace core::async {

namespace details {
    template <typename Select, typename T>
    struct conc_task_state {
        conc_task_state(Select* iselect): selector(iselect) {};
        conc_task_state(Select* iselect, task<T> task): selector(iselect), _task(core::mov(task)) {}

        Select*          selector   = nullptr;
        ignore_result<T> _task      = {};
        task<void>       background = {};
        bool             returned   = false;
    };
} // namespace details

class concurrent_buff_is_full : public std::exception {
public:
    concurrent_buff_is_full() = default;

    const char* what() const noexcept override {
        return "concurrent: cannot push new task - buffer is full";
    }
};

template <typename ReturnT, size_t MaxTasks>
class concurrent {
public:
    explicit concurrent(auto... tasks)
        requires(sizeof...(tasks) <= MaxTasks)
        : remaining_(sizeof...(tasks)) {
        (states.emplace_back(this, core::mov(tasks)), ...);
        for (size_t i = sizeof...(tasks); i < MaxTasks; ++i) {
            states.emplace_back(this);
        }

        for (size_t i = 0; i < remaining_; ++i) {
            background_tasks[i] = run_background(i);
        }
    }

    class awaiter {
    public:
        explicit awaiter(concurrent& selector): selector_(&selector) {}

        bool await_ready() const noexcept {
            if (selector_->remaining_ == 0)
                return true;
            return !selector_->ready_indices_.empty();
        }

        void await_suspend(std::coroutine_handle<> caller) noexcept {
            selector_->awaiting_handle_ = caller;
        }

        opt<size_t> await_resume() {
            if (selector_->remaining_ == 0) {
                return {};
            }

            size_t idx = selector_->ready_indices_.front();
            selector_->ready_indices_.pop();
            --selector_->remaining_;
            return idx;
        }

    private:
        concurrent* selector_;
    };

    task<opt<task<ReturnT>>> select() {
        auto ready_idx = co_await awaiter{*this};
        if (!ready_idx) {
            co_return {};
        }

        co_await background_tasks[*ready_idx];
        background_tasks[*ready_idx] = {};

        auto& state = states[*ready_idx];
        co_return core::mov(state._task.task);
    }

    void push(auto&& task) {
        if (remaining_ >= MaxTasks) {
            throw concurrent_buff_is_full{};
        }

        for (size_t i = 0; i < background_tasks.size(); ++i) {
            if (background_tasks[i].empty()) {
                ++remaining_;
                states[i]           = details::conc_task_state<concurrent, ReturnT>(this, core::mov(task));
                background_tasks[i] = run_background(i);
                return;
            }
        }

        throw concurrent_buff_is_full{};
    }

private:
    std::deque<details::conc_task_state<concurrent, ReturnT>> states;
    array<task<void>, MaxTasks>                            background_tasks;
    std::queue<size_t>                                        ready_indices_;
    std::coroutine_handle<>                                   awaiting_handle_ = nullptr;
    size_t                                                    remaining_;

    template <typename U = ReturnT>
    task<void> run_background(size_t idx) {
        auto& state = states[idx];
        co_await state._task;
        state.returned = true;
        notify(idx);
    }

    void notify(size_t idx) {
        auto& state = states[idx];
        ready_indices_.push(idx);
        if (awaiting_handle_) {
            auto handle      = awaiting_handle_;
            awaiting_handle_ = nullptr;
            handle.resume();
        }
    }
};

} // namespace core::async
