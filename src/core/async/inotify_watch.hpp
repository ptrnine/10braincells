#pragma once

#include <core/async/inotify_ctx.hpp>

#include <core/box.hpp>
#include <core/ranges/range.hpp>
#include <core/ring_buffer.hpp>

#define fwd(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace core::async {
class inotify_watch {
public:
    inotify_watch() = default;

    template <typename Str>
        requires null_term_string<decay<Str>>
    inotify_watch(Str&& path, sys::inotify_watch_flags flags, size_t ring_size = 32):
        _path(fwd(path)), _flags(flags), _events(boxed<wd_event_buffer>(ring_size)), _wd(current_inotify_ctx->add_watch(_path, _flags, _events.get()).get()) {}

    ~inotify_watch() {
        _destroy();
    }

    inotify_watch(inotify_watch&& iw) = default;
    inotify_watch& operator=(inotify_watch&& iw) noexcept {
        if (&iw == this) {
            return *this;
        }

        _destroy();

        _path   = mov(iw._path);
        _flags  = mov(iw._flags);
        _events = mov(iw._events);
        _wd     = mov(iw._wd);
        return *this;
    }

    auto events_view(size_t last_push_count = 0) const {
        return _events->view_from(last_push_count);
    }

    auto wait() const {
        return current_inotify_ctx->await_event_from(_wd);
    }

    class event_poller {
    public:
        event_poller() = default;
        event_poller(const inotify_watch& watch): _watch(&watch), _last_push_count(watch._events->push_count) {}

        auto wait() const {
            return _watch->wait();
        }

        auto poll_view() {
            auto [view, push_count] = _watch->events_view(_last_push_count);
            _last_push_count        = push_count;
            return view;
        }

        auto poll() {
            auto [view, push_count] = _watch->events_view(_last_push_count);
            _last_push_count        = push_count;
            return std::vector(view.begin(), view.end());
        }

    private:
        const inotify_watch* _watch           = nullptr;
        size_t               _last_push_count = 0;
    };

    event_poller poller() const {
        return {*this};
    }

private:
    void _destroy() {
        if (_wd.not_default()) {
            current_inotify_ctx->rm_watch(_wd);
            _wd.reset();
        }
    }

    std::string                                  _path;
    sys::inotify_watch_flags                     _flags;
    box<wd_event_buffer>                         _events;
    moveonly_trivial<sys::wd_t, sys::invalid_wd> _wd;
};
} // namespace core::async

#undef fwd
