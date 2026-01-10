#pragma once

#include <coroutine>

#include <core/basic_types.hpp>
#include <core/traits/remove_cvref.hpp>

#define fwd(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace core {
template <typename T>
struct awaitable_base {
    std::coroutine_handle<> _ctx;
    T                       _result;

    bool await_ready() const noexcept {
        return false;
    }

    T await_resume() const noexcept {
        return _result;
    }

    void resume(const T& value) {
        _result = value;
        _ctx.resume();
    }
};

template <typename F, typename T>
struct awaitable : awaitable_base<T> {
    awaitable(auto&& isuspend_handler): suspend_handler(fwd(isuspend_handler)) {}

    F suspend_handler;

    void await_suspend(std::coroutine_handle<> ctx) noexcept {
        this->_ctx = ctx;
        suspend_handler(*this);
    }
};

template <typename T>
auto make_awaitable(auto&& suspend_handler) {
    return awaitable<remove_cvref<decltype(suspend_handler)>, T>{fwd(suspend_handler)};
}

template <typename T>
awaitable_base<T>* to_awaitable_ptr(u64 ptr) {
    return (awaitable_base<T>*)ptr;
}
} // namespace core
