#pragma once

#include <core/any_holder.hpp>
#include <core/concepts/invocable.hpp>
#include <core/concepts/trivial_dtor.hpp>
#include <core/exception.hpp>
#include <core/meta/type.hpp>
#include <core/null.hpp>
#include <core/traits/declval.hpp>
#include <core/utility/move.hpp>

#define fwd(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace core {
class function_copy_ctor_error : public exception {
public:
    const char* what() const noexcept override {
        return "functonal object has no copy ctor";
    }
};

template <typename, size_t max_size, bool use_table = true>
struct function;

namespace details {
    template <typename>
    struct _is_function {
        static inline constexpr bool value = false;
    };
    template <typename T, size_t max_size, bool use_table>
    struct _is_function<function<T, max_size, use_table>> {
        static inline constexpr bool value = true;
    };
    template <typename T>
    struct _is_function<T&> : _is_function<T> {};
    template <typename T>
    struct _is_function<const T> : _is_function<T> {};

    template <typename T, typename F = decltype(&T::operator())>
    struct _call_operator_traits {
        static inline constexpr bool value = false;
    };

    template <typename T>
    struct _call_op_ok {
        using type = T;
        static inline constexpr bool value = true;
    };
    template <typename R, typename T, typename... Args>
    struct _call_operator_traits<T, R (T::*)(Args...) const> : _call_op_ok<R(Args...)> {};
    template <typename R, typename T, typename... Args>
    struct _call_operator_traits<T, R (T::*)(Args...)> : _call_op_ok<R(Args...)> {};
    template <typename R, typename T, typename... Args>
    struct _call_operator_traits<T, R (T::*)(Args...) noexcept> : _call_op_ok<R(Args...)> {};
    template <typename R, typename T, typename... Args>
    struct _call_operator_traits<T, R (T::*)(Args...) const noexcept> : _call_op_ok<R(Args...)> {};

    /* four-pointers implementation */
    template <bool use_table, size_t sz, typename R, typename... Args>
    struct _func_holder {
        using any_t = any_holder<sz>;

        template <typename Impl>
        void setup() noexcept {
            call  = &Impl::call;
            dtor  = &Impl::dtor;
            cctor = &Impl::cctor_from;
            mctor = &Impl::mctor_from;
        }

        void reset() {
            call = nullptr;
        }

        constexpr operator bool() const noexcept {
            return call;
        }

        constexpr bool need_dtor() const noexcept {
            return dtor;
        }

        R (*call)(any_t&, Args...)          = nullptr;
        void (*dtor)(any_t&)                = nullptr;
        void (*cctor)(any_t&, const any_t&) = [](any_t&, const any_t&) mutable {};
        void (*mctor)(any_t&, any_t&&)      = [](any_t&, any_t&&) mutable {};
    };

    enum class _func_mgr_action { dtor, cctor, mctor };

    /* two-pointers implementation (function pointer and pointer to manager function) */
    template <size_t sz, typename R, typename... Args>
    struct _func_holder<true, sz, R, Args...> {
        using any_t = any_holder<sz>;

        template <typename Impl>
        void setup() noexcept {
            call = &Impl::call;
            mgr  = &Impl::mgr;
        }

        void reset() {
            call = nullptr;
        }

        constexpr operator bool() const noexcept {
            return call;
        }

        constexpr bool need_dtor() const noexcept {
            return mgr;
        }

        R (*call)(any_t&, Args...)                          = nullptr;
        void (*mgr)(any_t&, const any_t&, _func_mgr_action) = [](any_t&, const any_t&, _func_mgr_action) mutable {};

        void dtor(any_t& f) const noexcept {
            mgr(f, f, _func_mgr_action::dtor);
        }

        void cctor(any_t& dst, const any_t& src) const {
            mgr(dst, src, _func_mgr_action::cctor);
        }

        void mctor(any_t& dst, any_t&& src) const {
            mgr(dst, src, _func_mgr_action::mctor);
        }
    };

    /* Implementation holder */
    template <size_t sz, typename F, typename R, typename... Args>
    struct _func_impl {
        inline static R call(any_holder<sz>& ah, Args... args) {
            return ah.template get<F>()(fwd(args)...);
        }

        inline static void dtor(any_holder<sz>& ah) {
            if constexpr (!trivial_dtor<F>)
                ah.template destruct<F>();
        }

        inline static void cctor_from(any_holder<sz>& dst, const any_holder<sz>& source) {
            if constexpr (requires {dst.template construct<F>(source.template get<F>());})
                dst.template construct<F>(source.template get<F>());
            else
                throw function_copy_ctor_error();
        }

        inline static void mctor_from(any_holder<sz>& dst, any_holder<sz>&& source) {
            if constexpr (requires {dst.template construct<F>(mov(source.template get<F>()));})
                dst.template construct<F>(mov(source.template get<F>()));
            else
                cctor_from(dst, source);
        }

        inline static void mgr(any_holder<sz>& dst, const any_holder<sz>& src, _func_mgr_action action) {
            switch (action) {
            case _func_mgr_action::dtor:
                dtor(dst);
                break;
            case _func_mgr_action::cctor:
                cctor_from(dst, src);
                break;
            case _func_mgr_action::mctor:
                mctor_from(dst, mov(const_cast<any_holder<sz>&>(src))); // NOLINT
                break;
            }
        }
    };
} // namespace details

template <typename R, typename... Args, size_t max_size, bool use_table>
struct function<R(Args...), max_size, use_table> {
private:
    template <typename F>
    using func_impl   = details::_func_impl<max_size, remove_const_ref<F>, R, Args...>;
    using func_holder = details::_func_holder<use_table, max_size, R, Args...>;

    template <typename F>
    void _set_impl() {
        _funcs.template setup<func_impl<F>>();
    }

    void _set_impl(const function& f) {
        _funcs = f._funcs;
    }

    void _delete() {
        if (_funcs.need_dtor()) {
            _funcs.dtor(_holder);
            //_funcs.reset();
        }
    }

public:
    constexpr function() = default;

    constexpr function(const function& f) {
        _set_impl(f);
        _funcs.cctor(_holder, f._holder);
    }
    constexpr function& operator=(const function& f) {
        if (&f == this)
            return *this;

        _delete();
        _set_impl(f);
        _funcs.cctor(_holder, f._holder);

        return *this;
    }

    constexpr function(function&& f) noexcept {
        _set_impl(f);
        _funcs.mctor(_holder, mov(f._holder));
        f._funcs.call = nullptr;
    }
    constexpr function& operator=(function&& f) noexcept {
        if (&f == this)
            return *this;

        _delete();
        _set_impl(f);
        _funcs.mctor(_holder, mov(f._holder));
        f._funcs.call = nullptr;

        return *this;
    }
 
    constexpr ~function() {
        _delete();
    }

    /* Emplace ctor */
    template <typename F> requires invocable_r<R, F, Args...>
    constexpr function(type_t<F>, auto&&... args) {
        _set_impl<F>();
        _holder.template construct<F>(fwd(args)...);
    }

    /* Function-like objects (with operator()) */
    template <typename F>
        requires (!details::_is_function<remove_ref<F>>::value) && invocable_r<R, F, Args...> //&&
        //requires (any_holder<max_size>& h, F&& f) { h.template construct<remove_const_ref<F>>(fwd(f)); }
    constexpr function(F&& callable) {
        _set_impl<remove_const_ref<F>>();
        _holder.template construct<remove_const_ref<F>>(fwd(callable));
    }

    /* Basic function */
    constexpr function(R(&func)(Args...)): function(&func) {}

    /* Member functions */
    template <typename Ret, typename Class, typename... ArgsT>
    constexpr function(Ret (Class::*member_func)(ArgsT...)):
        function([member_func](Class& instance, ArgsT... args) {
            return (instance.*member_func)(fwd(args)...);
        }) {}
    template <typename Ret, typename Class, typename... ArgsT>
    constexpr function(Ret (Class::*member_func)(ArgsT...) const):
        function([member_func](const Class& instance, ArgsT... args) {
            return (instance.*member_func)(fwd(args)...);
        }) {}
    template <typename Ret, typename Class, typename... ArgsT>
    constexpr function(Ret (Class::*member_func)(ArgsT...) noexcept):
        function([member_func](Class& instance, ArgsT... args) noexcept {
            return (instance.*member_func)(fwd(args)...);
        }) {}
    template <typename Ret, typename Class, typename... ArgsT>
    constexpr function(Ret (Class::*member_func)(ArgsT...) const noexcept):
        function([member_func](const Class& instance, ArgsT... args) noexcept {
            return (instance.*member_func)(fwd(args)...);
        }) {}

    /* Function-like object assignment operator */
    template <typename F>
        requires (!details::_is_function<F>::value) && invocable_r<R, F, Args...> //&&
        //requires (any_holder<max_size>& h, F&& f) { h.template construct<remove_const_ref<F>>(fwd(f)); }
    constexpr function& operator=(F&& function_obj) {
        _delete();
        _set_impl<remove_const_ref<F>>();
        _holder.template construct<remove_const_ref<F>>(fwd(function_obj));

        return *this;
    }

    /* Basic function assignment operator */
    constexpr function& operator=(R(&func)(Args...)) {
        return *this = &func;
    }

    /* Member function assignment operators */
    template <typename Class>
    constexpr function& operator=(R (Class::*member_func)(Args...)) {
        return *this = [member_func](Class& instance, Args... args) {
            return (instance.*member_func)(fwd(args)...);
        };
    }
    template <typename Class>
    constexpr function& operator=(R (Class::*member_func)(Args...) const) {
        return *this = [member_func](const Class& instance, Args... args) {
            return (instance.*member_func)(fwd(args)...);
        };
    }
    template <typename Class>
    constexpr function& operator=(R (Class::*member_func)(Args...) noexcept) {
        return *this = [member_func](Class& instance, Args... args) noexcept {
            return (instance.*member_func)(fwd(args)...);
        };
    }
    template <typename Class>
    constexpr function& operator=(R (Class::*member_func)(Args...) const noexcept) {
        return *this = [member_func](const Class& instance, Args... args) noexcept {
            return (instance.*member_func)(fwd(args)...);
        };
    }

    constexpr function(core::null_t): function() {}

    constexpr function& operator=(core::null_t) {
        reset();
        return *this;
    }

    void reset() {
        _delete();
        _funcs.reset();
    }

    template <typename F> requires invocable_r<R, F, Args...>
    F& emplace(auto&&... args) {
        _delete();
        _set_impl<F>();
        return _holder.template construct<F>(fwd(args)...);
    }

    constexpr R operator()(Args... args) const {
        return _funcs.call(_holder, fwd(args)...);
    }

    constexpr operator bool() const noexcept {
        return _funcs;
    }

private:
    mutable any_holder<max_size> _holder;
    func_holder                  _funcs;
};

/* Deduction guide for functions */
template <typename R, typename... Args>
function(R(Args...)) -> function<R(Args...), sizeof(R(*)(Args...)), false>;

/* Deduction guides for member functions */
template <typename R, typename Class, typename... Args>
function(R (Class::*)(Args...))
    -> function<R(Class&, Args...),
                sizeof([f = declval<R (Class::*)(Args...)>()](Class& c, Args... a) mutable { return (c.*f)(a...); }),
                false>;

template <typename R, typename Class, typename... Args>
function(R (Class::*)(Args...) const)
    -> function<R(const Class&, Args...),
                sizeof([f = declval<R (Class::*)(Args...) const>()](const Class& c, Args... a) mutable {
                    return (c.*f)(a...);
                }),
                false>;

template <typename R, typename Class, typename... Args>
function(R (Class::*)(Args...) noexcept)
    -> function<R(Class&, Args...),
                sizeof([f = declval<R (Class::*)(Args...) noexcept>()](Class& c, Args... a) mutable {
                    return (c.*f)(a...);
                }),
                false>;

template <typename R, typename Class, typename... Args>
function(R (Class::*)(Args...) const noexcept)
    -> function<R(const Class&, Args...),
                sizeof([f = declval<R (Class::*)(Args...) const noexcept>()](const Class& c, Args... a) mutable {
                    return (c.*f)(a...);
                }),
                false>;

template <typename T> requires details::_call_operator_traits<T>::value
function(T) -> function<typename details::_call_operator_traits<T>::type, sizeof(T)>;
} // namespace core

#undef fwd
