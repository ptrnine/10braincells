#pragma once

#include <core/assert.hpp>
#include <core/async/sys/inotify_watch.hpp>
#include <sys/statx.hpp>

namespace core::async::util {
class dir_watch_singleton {
public:
    friend class dir_watcher;

    struct dir_id {
        auto operator<=>(const dir_id&) const noexcept = default;

        u32 major;
        u32 minor;
        u64 ino;
    };

    struct dir_data {
        inotify_watch watch;
        u32           users = 0;
    };

    dir_watch_singleton(const dir_watch_singleton&)            = delete;
    dir_watch_singleton& operator=(const dir_watch_singleton&) = delete;

    static dir_watch_singleton& instance() {
        static dir_watch_singleton inst;
        return inst;
    }

private:
    tuple<dir_id, inotify_watch*> ref_watch(const std::string& path, sys::inotify_watch_flags flags = {}, u32 ring_size = 32) {
        auto res = sys::statx(path, sys::statx_mask::ino | sys::statx_mask::type).get();
        if (res.mode.type() != sys::file_type::dir) {
            throw errc_exception{errc::enotdir};
        }

        auto [it, was_insert] = _ws.emplace(dir_id{.major = res.dev_major, .minor = res.dev_minor, .ino = res.ino}, inotify_watch{});
        if (was_insert) {
            flags |= sys::inotify_watch_flags::delete_self | sys::inotify_watch_flags::move_self;
            it->second.watch = inotify_watch(path, flags, ring_size);
        }
        ++it->second.users;
        return {it->first, &it->second.watch};
    }

    tuple<dir_id, inotify_watch*> ref_watch(const dir_id& id) {
        auto it = _ws.find(id);
        tbc_r_assert(it != _ws.end());

        ++(it->second.users);
        return {it->first, &it->second.watch};
    }

    void unref_watch(const dir_id& id) {
        auto pos = _ws.find(id);
        if (pos == _ws.end()) {
            return;
        }

        --(pos->second.users);
        if (pos->second.users == 0) {
            _ws.erase(pos);
        }
    }

    dir_watch_singleton()  = default;
    ~dir_watch_singleton() = default;

    std::map<dir_id, dir_data> _ws;
};

inline dir_watch_singleton& dir_watch() {
    return dir_watch_singleton::instance();
}

class dir_watcher {
public:
    dir_watcher() = default;

    dir_watcher(const std::string& path, sys::inotify_watch_flags flags = {}, u32 ring_size = 32) {
        auto [new_id, iw] = dir_watch().ref_watch(path, flags, ring_size);
        id                = new_id;
        _poller           = iw->poller();
    }

    dir_watcher(const dir_watcher& dw) {
        if (dw.id.is_default()) {
            return;
        }

        auto [new_id, iw] = dir_watch().ref_watch(dw.id);
        id                = new_id;
        _poller           = iw->poller();
    }

    dir_watcher& operator=(const dir_watcher& dw) {
        if (this == &dw) {
            return *this;
        }

        if (dw.id.is_default()) {
            _destroy();
            return *this;
        }

        auto [new_id, iw] = dir_watch().ref_watch(dw.id);
        id                = new_id;
        _poller           = iw->poller();

        return *this;
    }

    dir_watcher(dir_watcher&& dw) noexcept: id(mov(dw.id)), _poller(mov(dw._poller)) {}
    dir_watcher& operator=(dir_watcher&& dw) noexcept {
        if (this == &dw) {
            return *this;
        }

        _destroy();

        id      = mov(dw.id);
        _poller = mov(dw._poller);

        return *this;
    }

    ~dir_watcher() {
        _destroy();
    }

    auto poller() const {
        return _poller;
    }

private:
    void _destroy() {
        if (id.not_default()) {
            dir_watch().unref_watch(id);
        }
    }

    moveonly_trivial<dir_watch_singleton::dir_id, dir_watch_singleton::dir_id{.major = 0, .minor = 0, .ino = 0}> id;
    async::inotify_watch::event_poller                                                                           _poller;
};
} // namespace core::async::util
