#pragma once

#include <core/basic_types.hpp>
#include <core/opt.hpp>
#include <core/traits/is_same.hpp>
#include <coroutine>
#include <exception>

namespace core {
struct task_awaitable_final {
    bool await_ready() const noexcept {
        return false;
    }

    template <typename Promise>
    std::coroutine_handle<> await_suspend(std::coroutine_handle<Promise> handle) noexcept {
        // log.warn("final:suspend");
        if (handle.promise()._continuation)
            return handle.promise()._continuation;
        return std::noop_coroutine();
    }

    void await_resume() const noexcept {
        // log.warn("final:resume");
    }
};

template <typename Task, typename ResultT>
struct task_promise_type {
    std::coroutine_handle<> _continuation;
    opt<ResultT>            _result;
    std::exception_ptr      _exception;

    Task get_return_object() {
        return Task{std::coroutine_handle<task_promise_type>::from_promise(*this)};
    }

    void unhandled_exception() noexcept {
        //__builtin_printf("ZALUPA\n");
        _exception = std::current_exception();
    }

    void return_value(ResultT value) noexcept {
        //__builtin_printf("ZALUPA2 %s\n", __PRETTY_FUNCTION__);
        _result = mov(value);
        // log.warn("promise:return");
    }

    bool ready() const noexcept {
        //__builtin_printf("PROMISE VALUE2: %s\n", _result.has_value() ? "not null" : "null");
        //__builtin_printf("PROMISE EXCEPTION2: %s\n", _exception == nullptr ? "null" : "not null");
        return _result.has_value() || _exception;
    }

    std::suspend_never initial_suspend() noexcept {
        // log.warn("promise:initial_suspend");
        return {};
    }

    task_awaitable_final final_suspend() noexcept {
        // log.warn("promise:final_suspend: {}", _continuation.address());
        return {};
    }
};

template <typename Task>
struct task_promise_type<Task, void> {
    std::coroutine_handle<> _continuation;
    bool                    returned = false;
    std::exception_ptr      _exception;

    Task get_return_object() {
        return Task{std::coroutine_handle<task_promise_type>::from_promise(*this)};
    }

    void unhandled_exception() noexcept {
        _exception = std::current_exception();
    }

    void return_void() noexcept {
        returned = true;
        // log.warn("promise:return");
    }

    bool ready() const noexcept {
        return returned || _exception;
    }

    std::suspend_never initial_suspend() noexcept {
        // log.warn("promise:initial_suspend");
        return {};
    }

    task_awaitable_final final_suspend() noexcept {
        // log.warn("promise:final_suspend: {}", _continuation.address());
        return {};
    }
};

template <typename ResultT = void>
struct [[nodiscard]] task {
    using promise_type = task_promise_type<task, ResultT>;

    bool await_ready() noexcept {
        // log.warn("task:ready: {} {}", !_handle, _handle.done());
        if (!_handle || _handle.done()) {
            return true;
        } else if (_handle.promise().ready()) {
            //if constexpr (!is_same<ResultT, void>) {
            //    __builtin_printf("PROMISE VALUE: %s\n", _handle.promise()._result ? "not null" : "null");
            //}
            //__builtin_printf("PROMISE EXCEPTION: %s\n", _handle.promise()._exception == nullptr ? "null" : "not null");
            //_handle = {};
            return true;
        }
        return false;
    }

    void await_suspend(std::coroutine_handle<> calling) noexcept {
        // log.warn("task:suspend: {} {}", calling.address(), _handle.address());
        // return calling;
        _handle.promise()._continuation = calling;
        // return std::noop_coroutine_handle{};
        // return _handle;
    }

    template <typename T = ResultT>
        requires is_same<T, void>
    void await_resume() const noexcept {
        if (_handle.promise()._exception) {
            std::rethrow_exception(_handle.promise()._exception);
        }
    }

    template <typename T = ResultT>
        requires(!is_same<T, void>)
    T await_resume() const noexcept {
        if (_handle.promise()._exception) {
            std::rethrow_exception(_handle.promise()._exception);
        }

        //__builtin_printf("PROMISE VALUE %s: %s\n", __PRETTY_FUNCTION__, _handle.promise()._result ? "not null" : "null");
        return mov(_handle.promise()._result.value());
        // log.warn("task:resume");
    }

    task(std::coroutine_handle<promise_type> h): _handle(h) {}

    ~task() {
        if (_handle) {
            //__builtin_printf("destroy: %p\n", _handle.address());
            _handle.destroy();
        }
    }

    task(task&& t) noexcept: _handle(t._handle) {
        t._handle = {};
    }

    task& operator=(task&& t) noexcept {
        _handle   = t._handle;
        t._handle = {};
        return *this;
    }

    std::coroutine_handle<promise_type> _handle;
};
} // namespace core
