#pragma once

#include <compare>

#include <core/async/awaitable_type.hpp>

namespace core::async {
struct cancelation_point_t {
    static cancelation_point_t from_awaitable(void* awaitable, awaitable_type type) {
        return cancelation_point_t{pack_awaitable(u64(awaitable), type)};
    }

    explicit constexpr operator bool() const noexcept {
        return _awaitable_ptr != 0;
    }

    u64 get() const {
        return _awaitable_ptr;
    }

    u64 awaitable() const {
        return unpack_awaitable(_awaitable_ptr)[int_c<size_t(0)>];
    }

    awaitable_type type() const {
        return unpack_awaitable(_awaitable_ptr)[int_c<size_t(1)>];
    }

    void set(u64 awaitable_ptr, awaitable_type type) {
        _awaitable_ptr = pack_awaitable(awaitable_ptr, type);
    }

    constexpr auto operator<=>(const cancelation_point_t&) const noexcept = default;

    u64 _awaitable_ptr = 0;
};
} // namespace core::async
