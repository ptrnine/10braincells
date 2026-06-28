#pragma once

#include <queue>

#include <core/async/task.hpp>

namespace core::async {

namespace details {
    template <typename Select, typename T>
    struct conc_task_state_base {
        Select*            selector = nullptr;
        task<T>            _task;
        task<void>         background;
        std::exception_ptr exception = nullptr;
    };

    template <typename Select, typename T>
    struct conc_task_state : conc_task_state_base<Select, T> {
        conc_task_state(Select* selector, task<T> task): conc_task_state_base<Select, T>{selector, core::move(task)} {}
        opt<T> result;
    };

    template <typename Select>
    struct conc_task_state<Select, void> : public conc_task_state_base<Select, void> {
        conc_task_state(Select* selector, task<void> task): conc_task_state_base<Select, void>{selector, core::move(task)} {}
        bool returned = false; // ???
    };
} // namespace details

template <typename ReturnT, size_t TaskCount>
class concurrent {
public:
    explicit concurrent(auto... tasks): remaining_(sizeof...(tasks)) {
        (states.emplace_back(this, core::mov(tasks)), ...);

        for (size_t i = 0; i < background_tasks.size(); ++i) {
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
            //if (!selector_->ready_indices_.empty()) {
            //    selector_->awaiting_handle_ = nullptr;
            //    caller.resume();
            //}
        }

        template <typename U = ReturnT>
        auto await_resume() {
            if (selector_->remaining_ == 0) {
                if constexpr (is_same<void, U>)
                    return false;
                else
                    return opt<U>{};
            }

            size_t idx = selector_->ready_indices_.front();
            selector_->ready_indices_.pop();
            auto& state  = selector_->states[idx];
            --selector_->remaining_;

            if (state.exception) {
                std::rethrow_exception(state.exception);
            }

            if constexpr (!is_same<void, U>) {
                return mov(state.result);
            } else {
                return state.returned;
            }
        }

    private:
        concurrent* selector_;
    };

    template <typename U = ReturnT> requires (!is_same<void, U>)
    task<opt<ReturnT>> async_select() {
        auto result = co_await awaiter{*this};
        if (remaining_ == 0) {
            for (auto& t : background_tasks) {
                if (!t.empty()) {
                    co_await t;
                    t = {};
                }
            }
        }
        co_return result;
    }

    template <typename U = ReturnT> requires (is_same<void, U>)
    task<bool> async_select() {
        auto result = co_await awaiter{*this};
        if (remaining_ == 0) {
            for (auto& t : background_tasks) {
                if (!t.empty()) {
                    co_await t;
                    t = {};
                }
            }
        }
        co_return result;
    }

private:
    std::deque<details::conc_task_state<concurrent, ReturnT>> states;
    array<task<void>, TaskCount>                              background_tasks;
    std::queue<size_t>                                        ready_indices_;
    std::coroutine_handle<>                                   awaiting_handle_ = nullptr;
    size_t                                                    remaining_;

    template <typename U = ReturnT>
    task<void> run_background(size_t idx) {
        auto& state = states[idx];
        try {
            if constexpr (is_same<U, void>) {
                co_await state._task;
                state.returned = true;
            } else {
                state.result = co_await state._task;
            }
        } catch (...) {
            state.exception = std::current_exception();
        }
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
