#pragma once

#include <core/nostd/concepts/assign.hpp>
#include <core/nostd/concepts/ctor.hpp>
#include <core/nostd/concepts/inheritance.hpp>
#include <core/nostd/concepts/nothrow_assign.hpp>
#include <core/nostd/concepts/nothrow_ctor.hpp>
#include <core/nostd/concepts/same_as.hpp>
#include <core/nostd/concepts/trivial_assign.hpp>
#include <core/nostd/concepts/trivial_ctor.hpp>
#include <core/nostd/concepts/trivial_dtor.hpp>
#include <core/nostd/meta/init.hpp>
#include <core/nostd/traits/is_ref.hpp>
#include <core/nostd/traits/remove_ref.hpp>
#include <core/null.hpp>

#define fwd(what) static_cast<decltype(what)>(what)

namespace core
{

namespace details
{
    struct opt_base {};
} // namespace details

template <typename T>
concept optional = derived_from<details::opt_base, T>;

namespace details
{
    template <typename T>
    struct opt_holder_base;

    template <trivial_dtor T>
    struct opt_holder_base<T> : opt_base {
        constexpr opt_holder_base() noexcept: _dummy() {}

        template <typename... Ts>
        constexpr explicit opt_holder_base(init_t, Ts&&... args): _value(fwd(args)...), _has_value(true) {}

        constexpr void reset() noexcept {
            _has_value = false;
        }

        union {
            char _dummy;
            T    _value;
        };
        bool _has_value = false;
    };

    template <typename T> requires (!trivial_dtor<T>)
    struct opt_holder_base<T> : opt_base {
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

        union {
            char _dummy;
            T    _value;
        };
        bool _has_value = false;
    };

    template <typename T>
    struct opt_holder;

    template <typename T> requires (!is_ref<T>)
    struct opt_holder<T> : opt_holder_base<T> {
        using opt_holder_base<T>::opt_holder_base;

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

        constexpr decltype(auto) get() & noexcept {
            return this->_value;
        }

        constexpr decltype(auto) get() const& noexcept {
            return this->_value;
        }

        constexpr decltype(auto) get() && noexcept {
            return mov(this->_value);
        }

        constexpr decltype(auto) get() const&& noexcept {
            return mov(this->_value);
        }
    };

    template <typename D, typename>
    struct opt_cctor : D {
        using D::D;
        constexpr opt_cctor() noexcept                   = default;
        constexpr opt_cctor(const opt_cctor&)            = delete;
        constexpr opt_cctor& operator=(const opt_cctor&) = default;
        constexpr opt_cctor(opt_cctor&&)                 = default;
        constexpr opt_cctor& operator=(opt_cctor&&)      = default;
    };

    template <typename D, trivial_copy_ctor T>
    struct opt_cctor<D, T> : D {
        using D::D;
        constexpr opt_cctor() noexcept                                         = default;
        constexpr opt_cctor(const opt_cctor& r) noexcept(nothrow_copy_ctor<T>) = default;
        constexpr opt_cctor& operator=(const opt_cctor&)                       = default;
        constexpr opt_cctor(opt_cctor&&)                                       = default;
        constexpr opt_cctor& operator=(opt_cctor&&)                            = default;
    };

    template <typename D, copy_ctor T> requires(!trivial_copy_ctor<T>)
    struct opt_cctor<D, T> : D {
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
    struct opt_casgn : D {
        using D::D;
        constexpr opt_casgn() noexcept                   = default;
        constexpr opt_casgn(const opt_casgn&)            = default;
        constexpr opt_casgn& operator=(const opt_casgn&) = delete;
        constexpr opt_casgn(opt_casgn&&)                 = default;
        constexpr opt_casgn& operator=(opt_casgn&&)      = default;
    };

    template <typename D, trivial_copy_assign T>
    struct opt_casgn<D, T> : D {
        using D::D;
        constexpr opt_casgn() noexcept                                                      = default;
        constexpr opt_casgn(const opt_casgn&)                                               = default;
        constexpr opt_casgn& operator=(const opt_casgn& r) noexcept(nothrow_copy_assign<T>) = default;
        constexpr opt_casgn(opt_casgn&&)                                                    = default;
        constexpr opt_casgn& operator=(opt_casgn&&)                                         = default;
    };

    template <typename D, copy_assign T> requires(!trivial_copy_assign<T>)
    struct opt_casgn<D, T> : D {
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
    struct opt_mctor : D {
        using D::D;
        constexpr opt_mctor() noexcept                   = default;
        constexpr opt_mctor(const opt_mctor&)            = default;
        constexpr opt_mctor& operator=(const opt_mctor&) = default;
        constexpr opt_mctor(opt_mctor&&)                 = delete;
        constexpr opt_mctor& operator=(opt_mctor&&)      = default;
    };

    template <typename D, trivial_move_ctor T>
    struct opt_mctor<D, T> : D {
        using D::D;
        constexpr opt_mctor() noexcept                                    = default;
        constexpr opt_mctor(const opt_mctor&)                             = default;
        constexpr opt_mctor& operator=(const opt_mctor&)                  = default;
        constexpr opt_mctor(opt_mctor&& r) noexcept(nothrow_move_ctor<T>) = default;
        constexpr opt_mctor& operator=(opt_mctor&&)                       = default;
    };

    template <typename D, move_ctor T> requires(!trivial_move_ctor<T>)
    struct opt_mctor<D, T> : D {
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
    struct opt_masgn : D {
        using D::D;
        constexpr opt_masgn() noexcept                   = default;
        constexpr opt_masgn(const opt_masgn&)            = default;
        constexpr opt_masgn& operator=(const opt_masgn&) = default;
        constexpr opt_masgn(opt_masgn&&)                 = default;
        constexpr opt_masgn& operator=(opt_masgn&&)      = delete;
    };

    template <typename D, trivial_move_assign T>
    struct opt_masgn<D, T> : D {
        using D::D;
        constexpr opt_masgn() noexcept                                                 = default;
        constexpr opt_masgn(const opt_masgn&)                                          = default;
        constexpr opt_masgn& operator=(const opt_masgn&)                               = default;
        constexpr opt_masgn(opt_masgn&&)                                               = default;
        constexpr opt_masgn& operator=(opt_masgn&& r) noexcept(nothrow_move_assign<T>) = default;
    };

    template <typename D, move_assign T> requires(!trivial_move_assign<T>)
    struct opt_masgn<D, T> : D {
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
} // namespace details

template <typename T> requires (!(same_as<T, null_t> || same_as<T, decltype(nullptr)> || same_as<T, init_t>))
class opt : details::opt_impl<T> {
public:
    using base = details::opt_impl<T>;

    constexpr opt() noexcept = default;
    constexpr opt(const opt&) = default;
    constexpr opt(opt&&) = default;
    constexpr opt(null_t) noexcept {}

    template <typename... Ts>
    constexpr opt(init_t, Ts&&... args): base(init, fwd(args)...) {}

    template <typename U> requires convertible_to<U, T> && ctor<T, U>
    constexpr opt(U&& val): base(init, fwd(val)) {}

    template <typename U> requires (!convertible_to<U, T>) && ctor<T, U>
    explicit constexpr opt(U&& val): base(init, fwd(val)) {}

    constexpr opt& operator=(const opt&) = default;
    constexpr opt& operator=(opt&&) = default;
    constexpr opt& operator=(null_t) noexcept {
        this->reset();
        return *this;
    }

    constexpr bool has_value() const noexcept {
        return this->_has_value;
    }
};
} // namespace core

#undef fwd
