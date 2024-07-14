#pragma once

#include "basic_types.hpp"

#define fwd(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace core {
template <size_t size>
struct any_holder {
    void* data() noexcept {
        return _data;
    }

    const void* data() const noexcept {
        return _data;
    }

    template <typename T>
    T& get() & noexcept {
        return *static_cast<T*>(data());
    }

    template <typename T>
    const T& get() const& noexcept {
        return *static_cast<const T*>(data());
    }

    template <typename T, typename... Args>
        requires (sizeof(T) <= size) &&
        requires (Args&&... args) { *::new (static_cast<void*>(nullptr)) T(fwd(args)...); }
    T& construct(Args&&... args) {
        return *::new (data()) T(fwd(args)...);
    }

    template <typename T>
    void destruct() noexcept {
        get<T>().~T();
    }

    char _data[size];
};
} // namespace core

#undef fwd
