#pragma once

#include <coroutine>

#include <core/async/cancelation_point.hpp>
#include <core/coro/coro_handle_metainfo.hpp>
#include <core/opt.hpp>

namespace core {
template <typename T>
struct async_generator_promise;

template <typename T>
struct async_generator {
    using promise_type = async_generator_promise<T>;
    std::coroutine_handle<promise_type> _handle;

    async_generator(std::coroutine_handle<promise_type> h): _handle(h) {}
    ~async_generator() {
        if (_handle)
            _handle.destroy();
    }

    async_generator(async_generator&& other) noexcept: _handle(other._handle) {
        other._handle = {};
    }

    async_generator& operator=(async_generator&& other) noexcept {
        if (this != &other) {
            if (_handle)
                _handle.destroy();
            _handle = other._handle;
            other._handle = {};
        }
        return *this;
    }

    // Awaitable interface
    bool await_ready() const noexcept {
        return false;
    }

    template <typename Promise>
    void await_suspend(std::coroutine_handle<Promise> caller) noexcept {
        if (_handle.done()) {
            caller.resume();
            return;
        }
        _handle.promise()._waiter = caller;
        caller.promise()._cancelation_point = _handle.promise()._cancelation_point;
        _handle.resume();
    }

    opt<T> await_resume() {
        if (_handle.promise()._exception)
            std::rethrow_exception(_handle.promise()._exception);

        if (_handle.done())
            return {};

        auto val = std::move(_handle.promise()._current_value);
        _handle.promise()._current_value.reset();

        return val;
    }
};

template <typename T>
struct async_generator_promise {
    core::opt<T>            _current_value;
    std::exception_ptr      _exception;
    std::coroutine_handle<>    _waiter;
    async::cancelation_point_t _cancelation_point;

#ifdef CORO_METAINFO
    coro_handle_metainfo _metainfo;
#endif

    void set_metainfo([[maybe_unused]] coro_handle_metainfo metainfo) {
#ifdef CORO_METAINFO
        _metainfo = metainfo;
#endif
    }

    async_generator<T> get_return_object() noexcept {
        return async_generator<T>{std::coroutine_handle<async_generator_promise>::from_promise(*this)};
    }

    std::suspend_always initial_suspend() noexcept {
        return {};
    }

    struct awaiter {
        async_generator_promise& _prom;

        bool await_ready() const noexcept {
            return false;
        }
        void await_suspend(std::coroutine_handle<async_generator_promise>) noexcept {
            if (_prom._waiter) {
                auto w        = _prom._waiter;
                _prom._waiter = nullptr;
                w.resume();
            }
        }
        void await_resume() noexcept {}
    };

    auto final_suspend() noexcept {
        return awaiter{*this};
    }

    void unhandled_exception() noexcept {
        _exception = std::current_exception();
    }

    void return_void() noexcept {}

    auto yield_value(T value) noexcept {
        _current_value = std::move(value);
        return awaiter{*this};
    }
};
} // namespace core
