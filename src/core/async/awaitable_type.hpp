#pragma once

#include <core/robin_map.hpp>
#include <core/tuple.hpp>

#include <string_view>

namespace core::async {
static inline constexpr size_t awaitable_type_bits = 4;
enum class awaitable_type : u8 {
    uring            = 0,
    uring_threaded   = 1,
    inotify_wd_event = 2,
};

tuple<u64, awaitable_type> unpack_awaitable(u64 awaitable_ptr) {
    constexpr u64 shift = 64 - awaitable_type_bits;
    constexpr u64 mask  = ~((~u64(0) >> shift) << shift);
    return tuple{awaitable_ptr & mask, awaitable_type{u8(awaitable_ptr >> (64 - awaitable_type_bits))}};
}

u64 pack_awaitable(u64 awaitable_ptr, awaitable_type type) {
    constexpr u64 shift = 64 - awaitable_type_bits;
    return awaitable_ptr | u64(type) << shift;
}
} // namespace core::async

namespace core {
inline constexpr std::string_view to_string(async::awaitable_type value) {
    constexpr auto map = [] {
        core::static_int_map<u8, std::string_view, 3> m;
        m.emplace(u8(async::awaitable_type::uring), "uring");
        m.emplace(u8(async::awaitable_type::uring_threaded), "uring_threaded");
        m.emplace(u8(async::awaitable_type::inotify_wd_event), "inotify_wd_event");
        return m;
    }();
    return map.at(u8(value));
}
} // namespace core
