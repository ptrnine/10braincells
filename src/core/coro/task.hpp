#pragma once

#include <coroutine>
#include <exception>
#include <list>

#include <core/async/cancelation_point.hpp>
#include <core/basic_types.hpp>
#include <core/coro/coro_handle_metainfo.hpp>
#include <core/function.hpp>
#include <core/opt.hpp>
#include <core/traits/is_same.hpp>

#include <util/log.hpp>

#include <sys/syscall.hpp>

namespace core {
struct task_awaitable_final {
    bool await_ready() const noexcept {
        return false;
    }

    template <typename Promise>
    std::coroutine_handle<> await_suspend(std::coroutine_handle<Promise> handle) noexcept {
        if (handle.promise()._continuation)
            return handle.promise()._continuation;
        return std::noop_coroutine();
    }

    void await_resume() const noexcept {}
};

template <typename Task, typename ResultT>
struct task_promise_type {
    std::coroutine_handle<>    _continuation;
    async::cancelation_point_t _cancelation_point;
    opt<ResultT>               _result;
    std::exception_ptr         _exception;

#ifdef CORO_METAINFO
    coro_handle_metainfo _metainfo;
#endif

    void set_metainfo([[maybe_unused]] coro_handle_metainfo metainfo) {
#ifdef CORO_METAINFO
        _metainfo = metainfo;
#endif
    }

    Task get_return_object() {
        return Task{std::coroutine_handle<task_promise_type>::from_promise(*this)};
    }

    void unhandled_exception() noexcept {
        _exception = std::current_exception();
    }

    void return_value(ResultT value) noexcept {
        _result = mov(value);
    }

    bool ready() const noexcept {
        return _result.has_value() || _exception;
    }

    std::suspend_never initial_suspend() noexcept {
        return {};
    }

    task_awaitable_final final_suspend() noexcept {
        return {};
    }
};

template <typename Task>
struct task_promise_type<Task, void> {
    std::coroutine_handle<>    _continuation;
    async::cancelation_point_t _cancelation_point;
    bool                       returned = false;
    std::exception_ptr         _exception;

#ifdef CORO_METAINFO
    coro_handle_metainfo _metainfo;
#endif

    void set_metainfo([[maybe_unused]] coro_handle_metainfo metainfo) {
#ifdef CORO_METAINFO
        _metainfo = metainfo;
#endif
    }

    Task get_return_object() {
        return Task{std::coroutine_handle<task_promise_type>::from_promise(*this)};
    }

    void unhandled_exception() noexcept {
        _exception = std::current_exception();
    }

    void return_void() noexcept {
        returned = true;
    }

    bool ready() const noexcept {
        return returned || _exception;
    }

    std::suspend_never initial_suspend() noexcept {
        return {};
    }

    task_awaitable_final final_suspend() noexcept {
        return {};
    }
};

template <typename ResultT = void>
struct [[nodiscard]] task;

namespace details {
    class lost_tasks_singleton {
    public:
        lost_tasks_singleton(const lost_tasks_singleton&)            = delete;
        lost_tasks_singleton& operator=(const lost_tasks_singleton&) = delete;

        static lost_tasks_singleton& instance();

        template <typename T>
        void       push(task<T>&& task);
        task<void> wait_all();

    private:
        lost_tasks_singleton();
        ~lost_tasks_singleton();

        struct func_awaitable {
            function<task<void>(), 16> awaitable;
            async::cancelation_point_t cancelation_point;
#ifdef CORO_METAINFO
            coro_handle_metainfo metainfo;
#endif
        };

        std::list<func_awaitable> _awaitables;
    };
} // namespace details

template <typename ResultT>
struct [[nodiscard]] task {
    using result_type  = ResultT;
    using promise_type = task_promise_type<task, ResultT>;

    task(task&& t) noexcept: _handle(t._handle), _await_called(t._await_called) {
        t._handle = {};
    }

    task& operator=(task&& t) noexcept {
        _destroy();
        _handle       = t._handle;
        _await_called = t._await_called;
        t._handle     = {};
        return *this;
    }

    bool await_ready() noexcept {
        if (!_handle || _handle.done()) {
            _await_called = true;
            return true;
        } else if (_handle.promise().ready()) {
            _await_called = true;
            return true;
        }
        return false;
    }

    template <typename Promise>
    void await_suspend(std::coroutine_handle<Promise> calling) noexcept {
        _await_called                        = true;
        _handle.promise()._continuation      = calling;
        calling.promise()._cancelation_point = _handle.promise()._cancelation_point;

#ifdef CORO_METAINFO
        calling.promise()._metainfo = _handle.promise()._metainfo;
#endif
    }

    template <typename T = ResultT>
        requires is_same<T, void>
    void await_resume() const {
        if (_handle.promise()._exception) {
            std::rethrow_exception(_handle.promise()._exception);
        }
    }

    template <typename T = ResultT>
        requires(!is_same<T, void>)
    T await_resume() const {
        if (_handle.promise()._exception) {
            std::rethrow_exception(_handle.promise()._exception);
        }

        return mov(_handle.promise()._result.value());
    }

    task(std::coroutine_handle<promise_type> h = {}): _handle(h) {}

    ~task() {
        _destroy();
    }

    auto cancelation_point() const {
        if (_handle) {
            return _handle.promise()._cancelation_point;
        }
        return async::cancelation_point_t{};
    }

    coro_handle_metainfo metainfo() const {
#ifdef CORO_METAINFO
        if (_handle) {
            return _handle.promise()._metainfo;
        }
#else
        return {};
#endif
    }

    void _destroy() {
        if (_handle) {
            if (!_await_called) {
                glog().error("task{} {} started, but co_await not called", metainfo().to_string(), (void*)cancelation_point().get());
                details::lost_tasks_singleton::instance().push(task<ResultT>(mov(*this)));
                return;
            }
            _handle.destroy();
        }
    }

    task<sys::syscall_result<void>> cancel();

    std::coroutine_handle<promise_type> _handle;
    bool                                _await_called = false;
};

namespace details {
    inline lost_tasks_singleton& lost_tasks_singleton::instance() {
        thread_local lost_tasks_singleton inst;
        return inst;
    }

    template <typename T>
    inline void lost_tasks_singleton::push(task<T>&& task) {
        auto                  cancelation_point = task.cancelation_point();
        [[maybe_unused]] auto metainfo          = task.metainfo();
        _awaitables.push_back({
            [t = mov(task)] mutable -> core::task<void> { co_await t; },
            cancelation_point,
#ifdef CORO_METAINFO
            metainfo,
#endif
        });
    }

    inline task<void> lost_tasks_singleton::wait_all() {
        while (!_awaitables.empty()) {
            auto w = mov(_awaitables.front());
            _awaitables.pop_front();
            try {
#ifdef CORO_METAINFO
                glog().warn("co_await lost task{} {}", w.metainfo.to_string(), (void*)w.cancelation_point.get());
#else
                glog().warn("co_await lost task {}", (void*)w.cancelation_point.get());
#endif
                co_await w.awaitable();
            } catch (const std::exception& e) {
                glog().error("exception while co_await lost task {}: {}", (void*)w.cancelation_point.get(), e.what());
            }
        }
        _awaitables.clear();
    }

    inline lost_tasks_singleton::lost_tasks_singleton()  = default;
    inline lost_tasks_singleton::~lost_tasks_singleton() = default;
} // namespace details

inline static auto& lost_tasks() {
    return details::lost_tasks_singleton::instance();
}
} // namespace core
