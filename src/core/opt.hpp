#pragma once

#include <core/basic_types.hpp>
#include <core/concepts/assign.hpp>
#include <core/concepts/ctor.hpp>
#include <core/concepts/inheritance.hpp>
#include <core/concepts/nothrow_assign.hpp>
#include <core/concepts/nothrow_ctor.hpp>
#include <core/concepts/same_as.hpp>
#include <core/concepts/trivial_assign.hpp>
#include <core/concepts/trivial_ctor.hpp>
#include <core/concepts/trivial_dtor.hpp>
#include <core/exception.hpp>
#include <core/meta/init.hpp>
#include <core/null.hpp>
#include <core/traits/is_ref.hpp>
#include <core/traits/remove_cv.hpp>
#include <core/traits/remove_ref.hpp>
#include <core/utility/move.hpp>

#define fwd(what) static_cast<decltype(what)>(what)

namespace core
{

namespace details
{
    struct opt_base {};
} // namespace details

template <typename T>
concept optional = derived_from<T, details::opt_base>;

template <typename T>
concept cref_optional = derived_from<remove_const_ref<T>, details::opt_base>;

namespace details
{
    template <typename>
    class opt_holder_base;

    template <trivial_dtor T>
    class opt_holder_base<T> : public opt_base {
    public:
        constexpr opt_holder_base() noexcept: _dummy() {}

        template <typename... Ts>
        constexpr explicit opt_holder_base(init_t, Ts&&... args): _value(fwd(args)...), _has_value(true) {}

        constexpr void reset() noexcept {
            _has_value = false;
        }

    protected:
        union {
            char _dummy;
            T    _value;
        };
        bool _has_value = false;
    };

    template <typename T> requires (!trivial_dtor<T>)
    class opt_holder_base<T> : public opt_base {
    public:
        constexpr opt_holder_base() noexcept: _dummy() {}

        template <typename... Ts>
        constexpr explicit opt_holder_base(init_t, Ts&&... args): _value(fwd(args)...), _has_value(true) {}

        constexpr ~opt_holder_base() {
            if (_has_value)
                _value.~T();
        }

        constexpr void reset() noexcept {
            if (_has_value) {
                _value.~T();
                _has_value = false;
            }
        }

    protected:
        union {
            char _dummy;
            T    _value;
        };
        bool _has_value = false;
    };

    template <typename T>
    class opt_holder;

    template <typename T> requires (!is_ref<T>)
    class opt_holder<T> : public opt_holder_base<T> {
    public:
        using opt_holder_base<T>::opt_holder_base;

    protected:
        template <typename... Ts>
        constexpr void init(Ts&&... args) {
            ::new(static_cast<void*>(&this->_value)) T(fwd(args)...);
            this->_has_value = true;
        }

        template <typename T2> requires optional<remove_const_ref<T2>>
        constexpr void init_from(T2&& opt) {
            if (opt._has_value)
                init(fwd(opt).get());
        }

        template <typename T2> requires optional<remove_const_ref<T2>>
        constexpr void assign_from(T2&& opt) {
            if (&opt == this)
                return;

            if (this->_has_value == opt._has_value) {
                if (this->_has_value)
                    this->_value = fwd(opt).get();
            }
            else {
                if (this->_has_value)
                    this->reset();
                else
                    init_from(fwd(opt));
            }

        }

    public:
        constexpr T& get() & noexcept {
            return this->_value;
        }

        constexpr const T& get() const& noexcept {
            return this->_value;
        }

        constexpr T&& get() && noexcept {
            return mov(this->_value);
        }

        constexpr const T&& get() const&& noexcept {
            return mov(this->_value);
        }
    };

    template <typename D, typename>
    class opt_cctor : public D {
    public:
        using D::D;
        constexpr opt_cctor() noexcept                   = default;
        constexpr opt_cctor(const opt_cctor&)            = delete;
        constexpr opt_cctor& operator=(const opt_cctor&) = default;
        constexpr opt_cctor(opt_cctor&&)                 = default;
        constexpr opt_cctor& operator=(opt_cctor&&)      = default;
    };

    template <typename D, trivial_copy_ctor T>
    class opt_cctor<D, T> : public D {
    public:
        using D::D;
        constexpr opt_cctor() noexcept                                         = default;
        constexpr opt_cctor(const opt_cctor& r) noexcept(nothrow_copy_ctor<T>) = default;
        constexpr opt_cctor& operator=(const opt_cctor&)                       = default;
        constexpr opt_cctor(opt_cctor&&)                                       = default;
        constexpr opt_cctor& operator=(opt_cctor&&)                            = default;
    };

    template <typename D, copy_ctor T> requires(!trivial_copy_ctor<T>)
    class opt_cctor<D, T> : public D {
    public:
        using D::D;
        constexpr opt_cctor() noexcept = default;
        constexpr opt_cctor(const opt_cctor& r) noexcept(nothrow_copy_ctor<T>): D() {
            this->init_from(r);
        }
        constexpr opt_cctor& operator=(const opt_cctor&) = default;
        constexpr opt_cctor(opt_cctor&&)                 = default;
        constexpr opt_cctor& operator=(opt_cctor&&)      = default;
    };

    template <typename D, typename>
    class opt_casgn : public D {
    public:
        using D::D;
        constexpr opt_casgn() noexcept                   = default;
        constexpr opt_casgn(const opt_casgn&)            = default;
        constexpr opt_casgn& operator=(const opt_casgn&) = delete;
        constexpr opt_casgn(opt_casgn&&)                 = default;
        constexpr opt_casgn& operator=(opt_casgn&&)      = default;
    };

    template <typename D, trivial_copy_assign T>
    class opt_casgn<D, T> : public D {
    public:
        using D::D;
        constexpr opt_casgn() noexcept                                                      = default;
        constexpr opt_casgn(const opt_casgn&)                                               = default;
        constexpr opt_casgn& operator=(const opt_casgn& r) noexcept(nothrow_copy_assign<T>) = default;
        constexpr opt_casgn(opt_casgn&&)                                                    = default;
        constexpr opt_casgn& operator=(opt_casgn&&)                                         = default;
    };

    template <typename D, copy_assign T> requires(!trivial_copy_assign<T>)
    class opt_casgn<D, T> : public D {
    public:
        using D::D;
        constexpr opt_casgn() noexcept        = default;
        constexpr opt_casgn(const opt_casgn&) = default;
        constexpr opt_casgn& operator=(const opt_casgn& r) noexcept(nothrow_copy_assign<T>) {
            this->assign_from(r);
            return *this;
        }
        constexpr opt_casgn(opt_casgn&&)            = default;
        constexpr opt_casgn& operator=(opt_casgn&&) = default;
    };

    template <typename D, typename>
    class opt_mctor : public D {
    public:
        using D::D;
        constexpr opt_mctor() noexcept                   = default;
        constexpr opt_mctor(const opt_mctor&)            = default;
        constexpr opt_mctor& operator=(const opt_mctor&) = default;
        constexpr opt_mctor(opt_mctor&&)                 = delete;
        constexpr opt_mctor& operator=(opt_mctor&&)      = default;
    };

    template <typename D, trivial_move_ctor T>
    class opt_mctor<D, T> : public D {
    public:
        using D::D;
        constexpr opt_mctor() noexcept                                    = default;
        constexpr opt_mctor(const opt_mctor&)                             = default;
        constexpr opt_mctor& operator=(const opt_mctor&)                  = default;
        constexpr opt_mctor(opt_mctor&& r) noexcept(nothrow_move_ctor<T>) = default;
        constexpr opt_mctor& operator=(opt_mctor&&)                       = default;
    };

    template <typename D, move_ctor T> requires(!trivial_move_ctor<T>)
    class opt_mctor<D, T> : public D {
    public:
        using D::D;
        constexpr opt_mctor() noexcept                   = default;
        constexpr opt_mctor(const opt_mctor&)            = default;
        constexpr opt_mctor& operator=(const opt_mctor&) = default;
        constexpr opt_mctor(opt_mctor&& r) noexcept(nothrow_move_ctor<T>): D() {
            this->init_from(mov(r));
        }
        constexpr opt_mctor& operator=(opt_mctor&&) = default;
    };

    template <typename D, typename>
    class opt_masgn : public D {
    public:
        using D::D;
        constexpr opt_masgn() noexcept                   = default;
        constexpr opt_masgn(const opt_masgn&)            = default;
        constexpr opt_masgn& operator=(const opt_masgn&) = default;
        constexpr opt_masgn(opt_masgn&&)                 = default;
        constexpr opt_masgn& operator=(opt_masgn&&)      = delete;
    };

    template <typename D, trivial_move_assign T>
    class opt_masgn<D, T> : public D {
    public:
        using D::D;
        constexpr opt_masgn() noexcept                                                 = default;
        constexpr opt_masgn(const opt_masgn&)                                          = default;
        constexpr opt_masgn& operator=(const opt_masgn&)                               = default;
        constexpr opt_masgn(opt_masgn&&)                                               = default;
        constexpr opt_masgn& operator=(opt_masgn&& r) noexcept(nothrow_move_assign<T>) = default;
    };

    template <typename D, move_assign T> requires(!trivial_move_assign<T>)
    class opt_masgn<D, T> : public D {
    public:
        using D::D;
        constexpr opt_masgn() noexcept                   = default;
        constexpr opt_masgn(const opt_masgn&)            = default;
        constexpr opt_masgn& operator=(const opt_masgn&) = default;
        constexpr opt_masgn(opt_masgn&&)                 = default;
        constexpr opt_masgn& operator=(opt_masgn&& r) noexcept(nothrow_move_assign<T>) {
            this->assign_from(mov(r));
            return *this;
        }
    };

    template <typename T>
    using opt_impl4 = opt_cctor<opt_holder<T>, T>;

    template <typename T>
    using opt_impl3 = opt_mctor<opt_impl4<T>, T>;

    template <typename T>
    using opt_impl2 = opt_casgn<opt_impl3<T>, T>;

    template <typename T>
    using opt_impl = opt_masgn<opt_impl2<T>, T>;

    struct opt_iterator_sentinel {};

    template <typename T>
    class opt_iterator {
    public:
        opt_iterator() = default;
        opt_iterator(T& value) noexcept: val(&value) {}

        constexpr T& operator*() noexcept {
            return *val;
        }

        constexpr const T& operator*() const noexcept {
            return *val;
        }

        constexpr T* operator->() noexcept {
            return val;
        }

        constexpr const T* operator->() const noexcept {
            return val;
        }

        constexpr bool operator==(opt_iterator_sentinel) const noexcept {
            return !val;
        }

        constexpr bool operator!=(opt_iterator_sentinel) const noexcept {
            return val;
        }

        opt_iterator& operator++() {
            val = nullptr;
            return *this;
        }

    private:
        T* val = nullptr;
    };
} // namespace details

class bad_opt_access : public exception {
public:
    bad_opt_access() = default;
    const char* what() const noexcept override {
        return "Bad optional access";
    }
};

template <typename T>
    requires(!(same_as<remove_const<T>, null_t> || same_as<remove_const<T>, decltype(nullptr)> ||
               same_as<remove_const<T>, init_t>))
class opt : public details::opt_impl<T> {
public:
    using base = details::opt_impl<T>;
    using base::base;

    constexpr opt() noexcept = default;
    constexpr opt(const opt&) requires copy_ctor<T> = default;
    constexpr opt(opt&&) noexcept requires move_ctor<T> = default;
    constexpr opt(null_t) noexcept {}
    constexpr opt(decltype(nullptr)) noexcept {}

    template <typename... Ts>
    constexpr opt(init_t, Ts&&... args): base(init, fwd(args)...) {}

    template <typename U> requires convertible_to<U, T> && ctor<T, U> && (!optional<remove_const_ref<U>>)
    constexpr opt(U&& val): base(init, fwd(val)) {}

    template <typename U> requires (!convertible_to<U, T>) && ctor<T, U> && (!optional<remove_const_ref<U>>)
    explicit constexpr opt(U&& val): base(init, fwd(val)) {}

    constexpr opt& operator=(const opt&) requires copy_assign<T> = default;
    constexpr opt& operator=(opt&&) noexcept requires move_assign<T> = default;
    constexpr opt& operator=(null_t) noexcept {
        this->reset();
        return *this;
    }
    constexpr opt& operator=(decltype(nullptr)) noexcept {
        this->reset();
        return *this;
    }

    template <typename U> requires ctor<T, U> && (!optional<remove_const_ref<U>>)
    constexpr opt& operator=(U&& val) {
        if (has_value())
            this->get() = fwd(val);
        else
            this->init(fwd(val));
        return* this;
    }

    constexpr T* operator->() {
        return &this->get();
    }

    constexpr const T* operator->() const {
        return &this->get();
    }

    constexpr T& operator*() & {
        return this->get();
    }

    constexpr const T& operator*() const& {
        return this->get();
    }

    constexpr T&& operator*() && {
        return mov(this->get());
    }

    constexpr const T&& operator*() const&& {
        return mov(this->get());
    }

    constexpr bool has_value() const noexcept {
        return this->_has_value;
    }

    constexpr bool empty() const noexcept {
        return !this->_has_value;
    }

    constexpr size_t size() const noexcept {
        return has_value() ? 1 : 0;
    }

    constexpr explicit operator bool() const noexcept {
        return has_value();
    }

    constexpr T& value() & {
        if (empty())
            throw bad_opt_access{};
        return this->get();
    }

    constexpr const T& value() const& {
        if (empty())
            throw bad_opt_access{};
        return this->get();
    }

    constexpr T&& value() && {
        if (empty())
            throw bad_opt_access{};
        return mov(this->get());
    }

    constexpr const T&& value() const&& {
        if (empty())
            throw bad_opt_access{};
        return mov(this->get());
    }

    template <typename U> requires copy_ctor<T> && convertible_to<U&&, T>
    constexpr T value_or(U&& default_value) const& {
        if (has_value())
            return this->get();
        else
           return static_cast<T>(fwd(default_value));
    }

    template <typename U> requires move_ctor<T> && convertible_to<U&&, T>
    constexpr T value_or(U&& default_value) && {
        if (has_value())
            return mov(this->get());
        else
            return static_cast<T>(fwd(default_value));
    }

    constexpr auto and_then(auto&& f) & requires requires { {fwd(f)(this->get())} -> cref_optional; } {
        if (has_value())
            return fwd(f)(this->get());
        else
            return remove_const_ref<decltype(fwd(f)(this->get()))>{};
    }

    constexpr auto and_then(auto&& f) && requires requires { {fwd(f)(mov(this->get()))} -> cref_optional; } {
        if (has_value())
            return fwd(f)(mov(this->get()));
        else
            return remove_const_ref<decltype(fwd(f)(mov(this->get())))>{};
    }

    constexpr auto and_then(auto&& f) const& requires requires { {fwd(f)(this->get())} -> cref_optional; } {
        if (has_value())
            return fwd(f)(this->get());
        else
            return remove_const_ref<decltype(fwd(f)(this->get()))>{};
    }

    constexpr auto and_then(auto&& f) const&& requires requires { {fwd(f)(mov(this->get()))} -> cref_optional; } {
        if (has_value())
            return fwd(f)(mov(this->get()));
        else
            return remove_const_ref<decltype(fwd(f)(mov(this->get())))>{};
    }

    constexpr opt or_else(auto&& f) const& requires requires { {fwd(f)()} -> cref_optional; } {
        if (has_value())
            return *this;
        else
            return fwd(f)();
    }

    constexpr opt or_else(auto&& f) && requires requires { {fwd(f)()} -> cref_optional; } {
        if (has_value())
            return mov(*this);
        else
            return fwd(f)();
    }

    constexpr auto map(auto&& f) & {
        if (has_value())
            return opt<remove_cv<decltype(fwd(f)(this->get()))>>{fwd(f)(this->get())};
        else
            return opt<remove_cv<decltype(fwd(f)(this->get()))>>{};
    }

    constexpr auto map(auto&& f) const& {
        if (has_value())
            return opt<remove_cv<decltype(fwd(f)(this->get()))>>{fwd(f)(this->get())};
        else
            return opt<remove_cv<decltype(fwd(f)(this->get()))>>{};
    }

    constexpr auto map(auto&& f) && {
        if (has_value())
            return opt<remove_cv<decltype(fwd(f)(mov(this->get())))>>{fwd(f)(mov(this->get()))};
        else
            return opt<remove_cv<decltype(fwd(f)(mov(this->get())))>>{};
    }

    constexpr auto map(auto&& f) const&& {
        if (has_value())
            return opt<remove_cv<decltype(fwd(f)(mov(this->get())))>>{fwd(f)(mov(this->get()))};
        else
            return opt<remove_cv<decltype(fwd(f)(mov(this->get())))>>{};
    }

    constexpr auto transform(auto&& f) & {
        return this->map(fwd(f));
    }

    constexpr auto transform(auto&& f) const& {
        return this->map(fwd(f));
    }

    constexpr auto transform(auto&& f) && {
        return this->map(fwd(f));
    }

    constexpr auto transform(auto&& f) const&& {
        return this->map(fwd(f));
    }

    constexpr details::opt_iterator<T> begin() {
        if (has_value())
            return {this->get()};
        else
            return {};
    }

    constexpr details::opt_iterator<const T> begin() const {
        if (has_value())
            return {this->get()};
        else
            return {};
    }

    constexpr details::opt_iterator_sentinel end() {
        return {};
    }

    constexpr details::opt_iterator_sentinel end() const {
        return {};
    }
};

template <typename T>
opt(T) -> opt<T>;
} // namespace core

#undef fwd
