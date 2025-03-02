#pragma once

#include <core/loophole.hpp>
#include "net/data_mapper.hpp"

#define Transferable(Derived) core::net::transferable_struct<core::lh::count(), Derived>

namespace core::net {
struct transferable_base {};

using type_idx_t = uint16_t;

template <size_t idx, typename derived>
struct transferable_struct : transferable_base {
    static_assert(!requires { lh::map_def<idx, derived>::map_get; });

    static constexpr derived create(auto&&... args) {
        return derived({.type_idx = idx}, static_cast<decltype(args)>(args)...);
    }

    constexpr auto as_bytes() const noexcept {
        return as_flat(static_cast<const derived&>(*this));
    }

    type_idx_t type_idx;
};

template <typename T>
concept transferable = std::is_base_of_v<transferable_base, T>;

template <typename U>
    requires requires(const U& v) {
        { v.size() } -> std::convertible_to<size_t>;
        { v.data() } -> std::convertible_to<const void*>;
    }
void transferable_dispatch(const U& data, auto&& handler) {
    size_t idx = map_as<type_idx_t>(data);
    lh::type_idx_dispatch(idx, [&](auto t) {
        if constexpr (requires { handler(map_as<decltype(*t)>(data)); })
            handler(map_as<decltype(*t)>(data));
    });
}

template <typename U>
    requires requires(U v) {
        { v.size() } -> std::convertible_to<size_t>;
        { v.data() } -> std::convertible_to<const void*>;
        { v.release() };
    }
void transferable_dispatch_and_release(U data, auto&& handler) {
    size_t idx = map_as<type_idx_t>(data);
    lh::type_idx_dispatch(idx, [&](auto t) {
        if constexpr (requires { handler(map_as<decltype(*t)>(data)); }) {
            auto mapped = map_as<decltype(*t)>(data);
            data.release();
            handler(std::move(mapped));
        }
    });
}
} // namespace core::net
