#pragma once

#include <core/concepts/assign.hpp>
#include <core/concepts/nothrow_assign.hpp>
#include <core/concepts/nothrow_ctor.hpp>
#include <core/concepts/trivial_assign.hpp>
#include <core/concepts/trivial_ctor.hpp>
#include <core/utility/move.hpp>

namespace core {
template <typename Traits, typename B, typename V>
struct ca_traits_cc : public B {
    using B::B;
    constexpr ca_traits_cc() noexcept                      = default;
    constexpr ca_traits_cc(const ca_traits_cc&)            = delete;
    constexpr ca_traits_cc& operator=(const ca_traits_cc&) = default;
    constexpr ca_traits_cc(ca_traits_cc&&)                 = default;
    constexpr ca_traits_cc& operator=(ca_traits_cc&&)      = default;
};

template <typename Traits, typename B, copy_ctor V>
struct ca_traits_cc<Traits, B, V> : public B {
    friend Traits;
    using B::B;
    constexpr ca_traits_cc() noexcept = default;
    constexpr ca_traits_cc(const ca_traits_cc& b) noexcept(nothrow_copy_ctor<V>): B() {
        Traits::cc(*this, b);
    }
    constexpr ca_traits_cc& operator=(const ca_traits_cc&) = default;
    constexpr ca_traits_cc(ca_traits_cc&&)                 = default;
    constexpr ca_traits_cc& operator=(ca_traits_cc&&)      = default;
};

template <typename Traits, typename B, trivial_copy_ctor V>
struct ca_traits_cc<Traits, B, V> : public B {
    using B::B;
    constexpr ca_traits_cc() noexcept                                          = default;
    constexpr ca_traits_cc(const ca_traits_cc&) noexcept(nothrow_copy_ctor<V>) = default;
    constexpr ca_traits_cc& operator=(const ca_traits_cc&)                     = default;
    constexpr ca_traits_cc(ca_traits_cc&&)                                     = default;
    constexpr ca_traits_cc& operator=(ca_traits_cc&&)                          = default;
};

template <typename Traits, typename B, typename V>
struct ca_traits_ca : B {
    using B::B;
    constexpr ca_traits_ca() noexcept                      = default;
    constexpr ca_traits_ca(const ca_traits_ca&)            = default;
    constexpr ca_traits_ca& operator=(const ca_traits_ca&) = delete;
    constexpr ca_traits_ca(ca_traits_ca&&)                 = default;
    constexpr ca_traits_ca& operator=(ca_traits_ca&&)      = default;
};

template <typename Traits, typename B, copy_assign V>
struct ca_traits_ca<Traits, B, V> : B {
    friend Traits;
    using B::B;
    constexpr ca_traits_ca() noexcept           = default;
    constexpr ca_traits_ca(const ca_traits_ca&) = default;
    constexpr ca_traits_ca& operator=(const ca_traits_ca& b) noexcept(nothrow_copy_assign<V>) {
        return Traits::ca(*this, b);
    }
    constexpr ca_traits_ca(ca_traits_ca&&)            = default;
    constexpr ca_traits_ca& operator=(ca_traits_ca&&) = default;
};

template <typename Traits, typename B, trivial_copy_assign V>
struct ca_traits_ca<Traits, B, V> : B {
    using B::B;
    constexpr ca_traits_ca() noexcept                                                       = default;
    constexpr ca_traits_ca(const ca_traits_ca&)                                             = default;
    constexpr ca_traits_ca& operator=(const ca_traits_ca&) noexcept(nothrow_copy_assign<V>) = default;
    constexpr ca_traits_ca(ca_traits_ca&&)                                                  = default;
    constexpr ca_traits_ca& operator=(ca_traits_ca&&)                                       = default;
};

template <typename Traits, typename B, typename V>
struct ca_traits_mc : B {
    using B::B;
    constexpr ca_traits_mc() noexcept                      = default;
    constexpr ca_traits_mc(const ca_traits_mc&)            = default;
    constexpr ca_traits_mc& operator=(const ca_traits_mc&) = default;
    constexpr ca_traits_mc(ca_traits_mc&&)                 = delete;
    constexpr ca_traits_mc& operator=(ca_traits_mc&&)      = default;
};

template <typename Traits, typename B, move_ctor V>
struct ca_traits_mc<Traits, B, V> : B {
    friend Traits;
    using B::B;
    constexpr ca_traits_mc() noexcept                      = default;
    constexpr ca_traits_mc(const ca_traits_mc&)            = default;
    constexpr ca_traits_mc& operator=(const ca_traits_mc&) = default;
    constexpr ca_traits_mc(ca_traits_mc&& b) noexcept(nothrow_move_ctor<V>): B() {
        Traits::mc(*this, mov(b));
    }
    constexpr ca_traits_mc& operator=(ca_traits_mc&&)      = default;
};

template <typename Traits, typename B, trivial_move_ctor V>
struct ca_traits_mc<Traits, B, V> : B {
    using B::B;
    constexpr ca_traits_mc() noexcept                                     = default;
    constexpr ca_traits_mc(const ca_traits_mc&)                           = default;
    constexpr ca_traits_mc& operator=(const ca_traits_mc&)                = default;
    constexpr ca_traits_mc(ca_traits_mc&&) noexcept(nothrow_move_ctor<V>) = default;
    constexpr ca_traits_mc& operator=(ca_traits_mc&&)                     = default;
};

template <typename Traits, typename B, typename V>
struct ca_traits_ma : B {
    using B::B;
    constexpr ca_traits_ma() noexcept                      = default;
    constexpr ca_traits_ma(const ca_traits_ma&)            = default;
    constexpr ca_traits_ma& operator=(const ca_traits_ma&) = default;
    constexpr ca_traits_ma(ca_traits_ma&&)                 = default;
    constexpr ca_traits_ma& operator=(ca_traits_ma&&)      = delete;
};

template <typename Traits, typename B, move_assign V>
struct ca_traits_ma<Traits, B, V> : B {
    friend Traits;
    using B::B;
    constexpr ca_traits_ma() noexcept                      = default;
    constexpr ca_traits_ma(const ca_traits_ma&)            = default;
    constexpr ca_traits_ma& operator=(const ca_traits_ma&) = default;
    constexpr ca_traits_ma(ca_traits_ma&&)                 = default;
    constexpr ca_traits_ma& operator=(ca_traits_ma&& b) noexcept(nothrow_move_assign<V>) {
        return Traits::ma(*this, mov(b));
    }
};

template <typename Traits, typename B, trivial_move_assign V>
struct ca_traits_ma<Traits, B, V> : B {
    using B::B;
    constexpr ca_traits_ma() noexcept                                                  = default;
    constexpr ca_traits_ma(const ca_traits_ma&)                                        = default;
    constexpr ca_traits_ma& operator=(const ca_traits_ma&)                             = default;
    constexpr ca_traits_ma(ca_traits_ma&&)                                             = default;
    constexpr ca_traits_ma& operator=(ca_traits_ma&&) noexcept(nothrow_move_assign<V>) = default;
};

template <typename Traits, typename Base, typename T>
using ca_traits4 = ca_traits_cc<Traits, Base, T>;

template <typename Traits, typename Base, typename T>
using ca_traits3 = ca_traits_mc<Traits, ca_traits4<Traits, Base, T>, T>;

template <typename Traits, typename Base, typename T>
using ca_traits2 = ca_traits_ca<Traits, ca_traits3<Traits, Base, T>, T>;

template <typename Traits, typename Base, typename T>
using ca_traits = ca_traits_ma<Traits, ca_traits2<Traits, Base, T>, T>;
} // namespace core
