#pragma once

#include <core/basic_types.hpp>
#include <core/concepts/nothrow_assign.hpp>
#include <core/concepts/nothrow_ctor.hpp>
#include <core/concepts/trivial_assign.hpp>
#include <core/concepts/trivial_ctor.hpp>
#include <core/concepts/trivial_dtor.hpp>
#include <core/utility/int_seq.hpp>

#define fwd(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace core {
namespace dtls {
    template <typename T, size_t S>
    struct fixed_vector_base {
        T* data() noexcept {
            return reinterpret_cast<T*>(_data);
        }

        const T* data() const noexcept {
            return reinterpret_cast<const T*>(_data);
        }

        auto begin() noexcept {
            return data();
        }

        auto begin() const noexcept {
            return data();
        }

        auto end() noexcept {
            return begin() + _size;
        }

        auto end() const noexcept {
            return begin() + _size;
        }

        constexpr size_t size() const noexcept {
            return _size;
        }

        T& operator[](size_t i) noexcept {
            return data()[i];
        }

        const T& operator[](size_t i) const noexcept {
            return data()[i];
        }

        T& front() noexcept {
            return *begin();
        }

        const T& front() const noexcept {
            return *begin();
        }

        T& back() noexcept {
            return *(begin() + (_size - 1));
        }

        const T& back() const noexcept {
            return *(begin() + (_size - 1));
        }

        char   _data[sizeof(T) * S];
        size_t _size = 0;
    };

#define defn(x) (*this)[i + x].~T()
#define defn4(x) defn(x); defn((x + 1)); defn((x + 2)); defn((x + 3))
#define defn16(x)   \
    defn(x);        \
    defn((x + 1));  \
    defn((x + 2));  \
    defn((x + 3));  \
    defn((x + 4));  \
    defn((x + 5));  \
    defn((x + 6));  \
    defn((x + 7));  \
    defn((x + 8));  \
    defn((x + 9));  \
    defn((x + 10)); \
    defn((x + 11)); \
    defn((x + 12)); \
    defn((x + 13)); \
    defn((x + 14)); \
    defn((x + 15))

#define gen_vec512(size)                                  \
    if constexpr (sizeof(T) == 1) {                       \
        for (size_t j = 64; j < size; i += 64, j += 64) { \
            defn16(0);                                    \
            defn16(16);                                   \
            defn16(32);                                   \
            defn16(48);                                   \
        }                                                 \
    }                                                     \
    if constexpr (sizeof(T) == 2) {                       \
        for (size_t j = 32; j < size; i += 32, j += 32) { \
            defn16(0);                                    \
            defn16(16);                                   \
        }                                                 \
    }                                                     \
    if constexpr (sizeof(T) == 4) {                       \
        for (size_t j = 16; j < size; i += 16, j += 16) { \
            defn16(0);                                    \
        }                                                 \
    }                                                     \
    if constexpr (sizeof(T) == 8) {                       \
        for (size_t j = 8; j < size; i += 8, j += 8) {    \
            defn4(0);                                     \
            defn4(4);                                     \
        }                                                 \
    }                                                     \
    if constexpr (sizeof(T) == 16) {                      \
        for (size_t j = 4; j < size; i += 4, j += 4) {    \
            defn4(0);                                     \
        }                                                 \
    }                                                     \
    if constexpr (sizeof(T) == 32) {                      \
        for (size_t j = 2; j < size; i += 2, j += 2) {    \
            defn(0);                                      \
            defn(1);                                      \
        }                                                 \
    }                                                     \
    (void)0

    template <typename T, size_t S>
    struct sv_dtor : fixed_vector_base<T, S> {
        sv_dtor() = default;
        void _destroy() {
            size_t i = 0;
            gen_vec512(this->_size);
            for (; i < this->_size; ++i)
                (*this)[i].~T();
        }
        ~sv_dtor() {
            _destroy();
        }
        sv_dtor(sv_dtor&&) = default;
        sv_dtor& operator=(sv_dtor&&) = default;
        sv_dtor(const sv_dtor&) = default;
        sv_dtor& operator=(const sv_dtor&) = default;
    };

    template <trivial_dtor T, size_t S>
    struct sv_dtor<T, S> : fixed_vector_base<T, S> {
        sv_dtor() = default;
        ~sv_dtor() = default;
        sv_dtor(sv_dtor&&) = default;
        sv_dtor& operator=(sv_dtor&&) = default;
        sv_dtor(const sv_dtor&) = default;
        sv_dtor& operator=(const sv_dtor&) = default;
    };

#undef defn
#define defn(x) ::new (this->data() + i + x) T(mov(v[i + x]))
    template <typename T, size_t S>
    struct sv_mctor : sv_dtor<T, S> {
        sv_mctor() = default;
        ~sv_mctor() = default;
        sv_mctor(sv_mctor&& v) noexcept(nothrow_copy_ctor<T>) {
            size_t i = 0;
            gen_vec512(v._size);
            for (; i < v._size; ++i)
                ::new (this->data() + i) T(mov(v[i]));
            this->_size = v._size;
        }
        sv_mctor& operator=(sv_mctor&&) = default;
        sv_mctor(const sv_mctor&) = default;
        sv_mctor& operator=(const sv_mctor&) = default;
    };

    template <trivial_move_ctor T, size_t S>
    struct sv_mctor<T, S> : sv_dtor<T, S> {
        sv_mctor() = default;
        ~sv_mctor() = default;
        sv_mctor(sv_mctor&&) = default;
        sv_mctor& operator=(sv_mctor&&) = default;
        sv_mctor(const sv_mctor&) = default;
        sv_mctor& operator=(const sv_mctor&) = default;
    };

    template <typename T, size_t S>
    struct sv_masgn : sv_mctor<T, S> {
        sv_masgn() = default;
        ~sv_masgn() = default;
        sv_masgn(sv_masgn&& v) = default;
        sv_masgn& operator=(sv_masgn&& v) noexcept(nothrow_move_assign<T>) {
            static_assert(nothrow_move_assign<T>);
            if constexpr (nothrow_move_assign<T>) {
                if constexpr (!trivial_dtor<T>)
                    this->_destroy();
                size_t i = 0;
                gen_vec512(v._size);
                for (; i < v._size; ++i)
                    ::new (this->data() + i) T(mov(v[i]));
                this->_size = v._size;
                return *this;
            }
            else {
                /* Not implemented */
            }
        }
        sv_masgn(const sv_masgn&) = default;
        sv_masgn& operator=(const sv_masgn&) = default;
    };

    template <trivial_move_assign T, size_t S>
    struct sv_masgn<T, S> : sv_mctor<T, S> {
        sv_masgn() = default;
        ~sv_masgn() = default;
        sv_masgn(sv_masgn&&) = default;
        sv_masgn& operator=(sv_masgn&&) = default;
        sv_masgn(const sv_masgn&) = default;
        sv_masgn& operator=(const sv_masgn&) = default;
    };

#undef defn
#define defn(x) ::new (this->data() + i + x) T(v[i + x])
    template <typename T, size_t S>
    struct sv_cctor : sv_masgn<T, S> {
        sv_cctor() = default;
        ~sv_cctor() = default;
        sv_cctor(sv_cctor&&) = default;
        sv_cctor& operator=(sv_cctor&&) = default;
        sv_cctor(const sv_cctor& v) noexcept(nothrow_copy_ctor<T>) {
            size_t i = 0;
            gen_vec512(v._size);
            for (; i < v._size; ++i)
                ::new (this->data() + i) T(v[i]);
            this->_size = v._size;
        }
        sv_cctor& operator=(const sv_cctor&) = default;
    };

    template <trivial_copy_ctor T, size_t S>
    struct sv_cctor<T, S> : sv_masgn<T, S> {
        sv_cctor() = default;
        ~sv_cctor() = default;
        sv_cctor(sv_cctor&&) = default;
        sv_cctor& operator=(sv_cctor&&) = default;
        sv_cctor(const sv_cctor&) = default;
        sv_cctor& operator=(const sv_cctor&) = default;
    };

    template <typename T, size_t S>
    struct sv_casgn : sv_cctor<T, S> {
        sv_casgn() = default;
        ~sv_casgn() = default;
        sv_casgn(sv_casgn&&) = default;
        sv_casgn& operator=(sv_casgn&&) = default;
        sv_casgn(const sv_casgn&) = default;
        sv_casgn& operator=(const sv_casgn& v) noexcept(nothrow_copy_assign<T>) {
            if constexpr (nothrow_copy_assign<T>) {
                if constexpr (!trivial_dtor<T>)
                    this->_destroy();
                size_t i = 0;
                gen_vec512(v._size);
                for (; i < v._size; ++i)
                    ::new (this->data() + i) T(v[i]);
                this->_size = v._size;
            }
            else {
                sv_casgn tmp(v);
                *this = mov(tmp);
            }
            return *this;
        }
    };
#undef gen_vec512
#undef defn16
#undef defn4
#undef defn

    template <nothrow_copy_assign T, size_t S>
    struct sv_casgn<T, S> : sv_cctor<T, S> {
        sv_casgn() = default;
        ~sv_casgn() = default;
        sv_casgn(sv_casgn&&) = default;
        sv_casgn& operator=(sv_casgn&&) = default;
        sv_casgn(const sv_casgn&) = default;
        sv_casgn& operator=(const sv_casgn&) = default;
    };
} // namespace dtls

template <typename T, size_t S>
struct fixed_vector : dtls::sv_casgn<T, S> {
    using dtls::sv_casgn<T, S>::sv_casgn;

    template <size_t... i>
    void _init(idx_seq<i...>, auto&&... values) {
        if constexpr ((trivial_ctor<T, decltype(values)> && ...))
            (((*this)[i] = values), ...);
        else
            (::new (this->data() + i) T(values), ...);
    }

    fixed_vector(auto&&... values) requires (sizeof...(values) <= S) {
        this->_size = sizeof...(values);
        _init(make_idx_seq<sizeof...(values)>(), fwd(values)...);
    }

    T& emplace_back(auto&&... args) {
        if constexpr (trivial_ctor<T, decltype(args)...> && trivial_move_ctor<T>)
            return (*this)[this->_size++] = T(fwd(args)...);
        else
            return *::new (this->data() + this->_size++) T(fwd(args)...);
    }

    constexpr bool empty() const noexcept {
        return this->size() == 0;
    }
};
} // namespace core

#undef fwd
