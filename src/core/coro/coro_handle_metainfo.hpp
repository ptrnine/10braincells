#pragma once

#include <core/async/awaitable_type.hpp>
#include <core/robin_map.hpp>

namespace core {
enum class async_task_type : u8 {
    unknown = 0,
    read,
    write,
    open,
    close,
    sleep,
    cancel,
    waitid,
    getdents,
    inotify_watch_wait,
    statx,
    spawn_child,
};

inline constexpr std::string_view to_string(async_task_type value) {
    constexpr auto map = [] {
        core::static_int_map<u8, std::string_view, 12> m;
        m.emplace(u8(async_task_type::unknown), "");
        m.emplace(u8(async_task_type::read), "read");
        m.emplace(u8(async_task_type::write), "write");
        m.emplace(u8(async_task_type::open), "open");
        m.emplace(u8(async_task_type::close), "close");
        m.emplace(u8(async_task_type::sleep), "sleep");
        m.emplace(u8(async_task_type::cancel), "cancel");
        m.emplace(u8(async_task_type::waitid), "waitid");
        m.emplace(u8(async_task_type::getdents), "getdents");
        m.emplace(u8(async_task_type::inotify_watch_wait), "inotify_watch::wait");
        m.emplace(u8(async_task_type::statx), "statx");
        m.emplace(u8(async_task_type::spawn_child), "spawn_child");
        return m;
    }();
    return map.at(u8(value));
}

struct coro_handle_metainfo {
    constexpr std::string to_string() const {
        if (task_type == async_task_type::unknown) {
            return "";
        }
        return "(" + std::string(core::to_string(awaitable_type)) + ":" + std::string(core::to_string(task_type)) + ")";
    }

    async::awaitable_type awaitable_type{0};
    async_task_type       task_type{0};
};
} // namespace core
