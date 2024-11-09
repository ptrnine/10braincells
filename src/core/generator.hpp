#pragma once

#include <coroutine>
#include <exception>

#include <core/var.hpp>
#include <variant>

namespace core {

template <typename T>
class generator {
public:
    struct promise_type {
        friend generator;

        using value_type = std::remove_reference_t<T>;

        generator<T> get_return_object() noexcept {
            return {std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        [[nodiscard]]
        constexpr std::suspend_always initial_suspend() const noexcept {
            return {};
        }

        [[nodiscard]]
        constexpr std::suspend_always final_suspend() const noexcept {
            return {};
        }

        std::suspend_always yield_value(auto&& value) noexcept {
            result = &value;
            return {};
        }

        void unhandled_exception() {
            result = std::current_exception();
        }

        void return_void() {}

        [[nodiscard]]
        value_type& get() const noexcept {
            return *result.get(core::type<value_type*>);
        }

        struct{} empty; // Prevent gcc bug
        var<value_type*, std::exception_ptr> result{type<value_type*>};
    };

    struct end_iterator {};

    class iterator {
    public:
        iterator() noexcept = default;
        iterator(std::coroutine_handle<promise_type> coro) noexcept: coroutine(coro) {}

        bool operator==(const end_iterator&) const noexcept {
            return !coroutine || coroutine.done();
        }

        bool operator!=(const end_iterator& e) const noexcept {
            return !(*this == e);
        }

        iterator& operator++() {
            coroutine.resume();
            if (coroutine.done() && coroutine.promise().result.index() == 1)
                std::rethrow_exception(coroutine.promise().result.get(core::type<std::exception_ptr>));
            return *this;
        }

        T& operator*() const noexcept {
            return coroutine.promise().get();
        }

        std::remove_reference_t<T>* operator->() const noexcept {
            return &operator*();
        }

    private:
        std::coroutine_handle<promise_type> coroutine = nullptr;
    };

    generator(generator&& gen) noexcept: handle(gen.handle) {
        gen.handle = nullptr;
    }

    generator& operator=(generator&& gen) noexcept {
        if (this == &gen)
            return *this;

        handle     = gen.handle;
        gen.handle = nullptr;

        return *this;
    }

    ~generator() {
        if (handle)
            handle.destroy();
    }

    iterator begin() {
        handle.resume();
        if (handle.done() && handle.promise().result.index() == 1)
            std::rethrow_exception(handle.promise().result.get(core::type<std::exception_ptr>));
        return {handle};
    }

    end_iterator end() noexcept {
        return {};
    }

private:
    generator(std::coroutine_handle<promise_type> h) noexcept: handle(h) {}

    std::coroutine_handle<promise_type> handle;
};
} // namespace core
