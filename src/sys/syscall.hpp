#pragma once

#include <core/concepts/number.hpp>
#include <core/errc_exception.hpp>
#include <core/traits/copy_cvref.hpp>
#include <core/traits/invoke.hpp>
#include <core/traits/is_ptr.hpp>
#include <core/traits/is_enum.hpp>
#include <core/var.hpp>
#include <sys/pthread_unsafe.hpp>

#include <syscall_arch.h>

#define fwd(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace sys
{
using core::copy_cvref, core::errc, core::errc_exception, core::invoke_result, core::is_ptr, core::number,
    core::overloaded, core::type, core::var, core::is_same, core::is_enum;

using sc_arg = long;

namespace details
{
    template <typename D, typename T>
    struct syscall_result_impl {
        constexpr syscall_result_impl(auto&&... args): _r(fwd(args)...) {}

        static constexpr D make_error(errc err) {
            return {err};
        }

        static constexpr D make_value(auto&&... args) {
            return {type<T>, fwd(args)...};
        }

        constexpr bool ok() const {
            return _r.index() == _r.index_of(type<T>);
        }

        constexpr errc error() const {
            return visit(_r,
                         overloaded{
                             [](const errc& err) { return err; },
                             [](auto&&) { return errc{0}; },
                         });
        }

        constexpr errc unsafe_error() const {
            return _r.unsafe_get(type<errc>);
        }

        constexpr auto&& unsafe_get(this auto&& it) {
            return fwd(it)._r.unsafe_get(type<T>);
        }

        void throw_if_error() const {
            if (_r.index() == _r.index_of(type<errc>))
                throw errc_exception(_r.unsafe_get(type<errc>));
        }

        constexpr void set(errc error) {
            _r = error;
        }

        var<T, errc> _r;
    };

    template <typename D, typename T> requires (number<T> || is_ptr<T> || is_enum<T>)
    struct syscall_result_impl<D, T> {
        //static_assert(sizeof(T) == 8);

        constexpr explicit syscall_result_impl(errc err): syscall_result_impl(-(sc_arg)err.code) {}
        constexpr syscall_result_impl(sc_arg arg = 0): _r(arg) {}

        static constexpr D make_error(errc err) {
            return {-(sc_arg)err.code};
        }

        static constexpr D make_value(const T& value) {
            return {reinterpret_cast<sc_arg>(value)};
        }

        constexpr bool ok() const {
            return (unsigned long)_r <= (~0UL - 4095UL);
        }

        constexpr errc error() const {
            if ((unsigned long)_r > (~0UL - 4095UL))
                return {-_r};
            return {0};
        }

        constexpr errc unsafe_error() const {
            return {-_r};
        }

        constexpr auto&& unsafe_get(this auto&& it) {
            /* XXX: Works on LE only */
            return reinterpret_cast<copy_cvref<decltype(it), T>>(*(T*)&(it._r));
        }

        void throw_if_error() const {
            if ((unsigned long)_r > (~0UL - 4095UL))
                throw errc_exception(errc{-_r});
        }

        constexpr void set(errc error) {
            _r = -(sc_arg)error.code;
        }

        sc_arg _r;
    };
} // namespace details

template <typename T>
struct syscall_result : details::syscall_result_impl<syscall_result<T>, T> {
    using details::syscall_result_impl<syscall_result<T>, T>::syscall_result_impl;

    constexpr explicit operator bool() const {
        return this->ok();
    }

    constexpr auto&& get(this auto&& it) {
        it.throw_if_error();
        return fwd(it).unsafe_get();
    }

    constexpr auto&& operator*(this auto&& it) {
        return fwd(it).unsafe_get();
    }

    constexpr auto operator->(this auto&& it) {
        return &fwd(it).unsafe_get();
    }

    auto map(this auto&& it, auto&& handler) {
        using new_t = invoke_result<decltype(handler), T>;
        if (it.ok())
            return syscall_result<new_t>::make_value(fwd(handler)(fwd(it).unsafe_get()));
        else
            return syscall_result<new_t>::make_error(it.unsafe_error());
    }
};

template <>
struct syscall_result<void> {
    constexpr syscall_result(errc error = {0}): _r(error) {}
    constexpr syscall_result(sc_arg sc_res): _r((unsigned long)sc_res > (~0UL - 4095UL) ? (int)-sc_res : 0) {}

    constexpr explicit operator bool() const {
        return ok();
    }

    constexpr bool ok() const {
        return !_r;
    }

    static constexpr syscall_result make_error(errc err) {
        return {err};
    }

    constexpr errc error() const {
        return _r;
    }

    constexpr errc unsafe_error() const {
        return _r;
    }

    void throw_if_error() const {
        if (_r)
            throw errc_exception(_r);
    }

    constexpr void set(errc error) {
        _r = error;
    }

    errc _r;
};

namespace details
{
    inline sc_arg _syscall(sc_arg sc_number) {
        return __syscall0(sc_number);
    }
    inline sc_arg _syscall(sc_arg sc_number, sc_arg a) {
        return __syscall1(sc_number, a);
    }
    inline sc_arg _syscall(sc_arg sc_number, sc_arg a, sc_arg b) {
        return __syscall2(sc_number, a, b);
    }
    inline sc_arg _syscall(sc_arg sc_number, sc_arg a, sc_arg b, sc_arg c) {
        return __syscall3(sc_number, a, b, c);
    }
    inline sc_arg _syscall(sc_arg sc_number, sc_arg a, sc_arg b, sc_arg c, sc_arg d) {
        return __syscall4(sc_number, a, b, c, d);
    }
    inline sc_arg _syscall(sc_arg sc_number, sc_arg a, sc_arg b, sc_arg c, sc_arg d, sc_arg e) {
        return __syscall5(sc_number, a, b, c, d, e);
    }
    inline sc_arg _syscall(sc_arg sc_number, sc_arg a, sc_arg b, sc_arg c, sc_arg d, sc_arg e, sc_arg f) {
        return __syscall6(sc_number, a, b, c, d, e, f);
    }
} // namespace details

/**
 * @brief System call
 *
 * Trap is a type functional object that will be constructed and called before the actual system call.
 * The result of Trap{}() constructed before system call and destructed after.
 *
 * @tparam T - result type
 * @tparam Trap - the type of functional object that will be constructed and called before the actual system call
 * @param sc_number - number (NR) of system call
 * @param args - arguments
 *
 * @return syscall_result with result or error
 */
template <typename R, typename Trap = void>
auto syscall(sc_arg sc_number, auto&&... args) {
    if constexpr (!is_same<Trap, void>) {
        Trap t;
        t.enter();
        auto res = details::_syscall(sc_number, (sc_arg)args...);
        t.exit(res);
        return syscall_result<R>(res);
    }
    else {
        return syscall_result<R>(details::_syscall(sc_number, (sc_arg)args...));
    }
}

struct trap_async_cancel {
    void enter() {
        /*
         * Enable async cancel for the system call
         * Exit if thread canceled
         */
        old = enable_async_cancel();
    }

    void exit(long) {
        disable_async_cancel(old);
    }

    uint old;
};
} // namespace sys

#undef fwd
