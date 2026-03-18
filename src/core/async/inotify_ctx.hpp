#pragma once

#include <core/constants.hpp>
#include <core/coro/coro_handle_metainfo.hpp>
#include <map>

#include <sys/close.hpp>
#include <sys/inotify.hpp>

#include <core/async/read.hpp>
#include <core/fixed_vector.hpp>
#include <core/ranges/range.hpp>
#include <core/ring_buffer.hpp>

namespace core::async {
static inline constexpr size_t inotify_event_buff_size     = (sizeof(sys::inotify_event) + constants::name_max) * 2;
static inline constexpr size_t inotify_max_events_per_read = inotify_event_buff_size / sizeof(sys::inotify_event);

struct wd_event {
    sys::inotify_watch_flags flags;
    std::string              name;
};

using wd_events = fixed_vector<wd_event, inotify_max_events_per_read>;

struct wd_event_buffer {
    wd_event_buffer(size_t ring_size): events(ring_size) {}

    void push(wd_event event) {
        ++push_count;
        events.push(mov(event));
    }

    auto view_from(size_t last_push_count) const {
        auto size  = push_count - last_push_count;
        auto start = events.begin();
        auto end   = events.end();
        if (size < events.size()) {
            start += diff_t(events.size() - size);
        }
        return tuple{range_holder{start, end}, push_count};
    }

    ring_buffer<wd_event> events;
    size_t                push_count = 0;
};

class inotify_ctx {
public:
    inotify_ctx() {
        fd = sys::inotify_init().get();
    }

    ~inotify_ctx() {
        if (fd != sys::invalid_fd) {
            sys::close(fd);
        }
    }

    inotify_ctx(inotify_ctx&&)            = delete;
    inotify_ctx& operator=(inotify_ctx&&) = delete;

    task<> run() {
        u8 buff[inotify_event_buff_size];

        std::vector<awaitable_base<sys::syscall_result<void>>*> resume_list;

        while (true) {
            auto res = sys::syscall_result<size_t>::make_error(errc::ecanceled);
            if (!canceled) {
                read_task = async::read(fd, buff);
                res = co_await read_task;
            }
            if (!res) {
                for (auto&& [_, consumer] : consumers) {
                    for (auto& awaitable : consumer.awaitables) {
                        awaitable->resume(res.error());
                    }
                }
                consumers.clear();
                if (res.error() == errc::ecanceled) {
                    break;
                }
                throw errc_exception(res.unsafe_error());
            }
            auto size = res.unsafe_get();

            for (size_t pos = 0; pos < size;) {
                auto event = (sys::inotify_event*)(buff + pos);
                pos += event->size();

                auto consumer_it = consumers.find(event->wd);
                if (consumer_it != consumers.end()) {
                    consumer_it->second.buff->push(
                        wd_event{
                            .flags = event->mask,
                            .name  = std::string(event->name()),
                        }
                    );
                    for (auto awaitable : consumer_it->second.awaitables) {
                        resume_list.push_back(awaitable);
                    }
                    consumer_it->second.awaitables.clear();
                }
            }

            for (auto awaitable : resume_list) {
                awaitable->resume(errc{});
            }
            resume_list.clear();
        }
    }

    task<> stop() {
        // Read task may already succedded (in this case cancel() will fail and return enoent)
        // So we need 'canceled' flag to stop run() anyways
        canceled = true;
        co_await read_task.cancel();

        sys::close(fd);
        fd = sys::invalid_fd;
    }

    template <typename Str> requires null_term_string<decay<Str>>
    auto add_watch(const Str& path, sys::inotify_watch_flags flags, wd_event_buffer* buff) {
        flags.unset(sys::inotify_watch_flag::mask_add);
        flags |= sys::inotify_watch_flag::mask_create;
        auto result = sys::inotify_add_watch(fd, path, flags);
        if (result) {
            consumers.emplace(result.unsafe_get(), consumer_state{.buff = buff, .awaitables = {}});
        }
        return result;
    }

    auto rm_watch(sys::wd_t wd) {
        auto result = sys::inotify_rm_watch(fd, wd);
        if (result) {
            auto it = consumers.find(wd);
            if (it != consumers.end()) {
                for (auto& awaitable : it->second.awaitables) {
                    awaitable->resume({errc::ecanceled});
                }
                consumers.erase(wd);
            }
        }
        return result;
    }

    auto await_event_from(sys::wd_t wd) {
        return make_awaitable<sys::syscall_result<void>>(
            [this, wd]<typename Promise>(awaitable_base<sys::syscall_result<void>>& awaitable, std::coroutine_handle<Promise>& caller) {
                subscribe_wd(wd, awaitable);
                caller.promise()._cancelation_point.set((u64)&awaitable, awaitable_type::inotify_wd_event);
                caller.promise().set_metainfo({awaitable_type::inotify_wd_event, async_task_type::inotify_watch_wait});
            }
        );
    }

    sys::syscall_result<size_t> cancel(u64 awaitable) {
        for (auto& consumer : consumers) {
            auto& awaitables = consumer.second.awaitables;
            auto  it         = std::ranges::find(awaitables, (awaitable_base<sys::syscall_result<void>>*)awaitable);
            if (it != awaitables.end()) {
                (*it)->resume({errc::ecanceled});
                awaitables.erase(it);
                return {1};
            }
        }
        return sys::syscall_result<size_t>{errc::enoent};
    }

private:
    void subscribe_wd(sys::wd_t wd, awaitable_base<sys::syscall_result<void>>& awaitable) {
        consumers[wd].awaitables.emplace_back(&awaitable);
    }
    struct consumer_state {
        wd_event_buffer*                                        buff;
        std::vector<awaitable_base<sys::syscall_result<void>>*> awaitables;
    };

    sys::fd_t                           fd = sys::invalid_fd;
    task<sys::syscall_result<size_t>>   read_task;
    std::map<sys::wd_t, consumer_state> consumers;
    bool                                canceled = false;
};

inline thread_local inotify_ctx* current_inotify_ctx = nullptr;
} // namespace core::async
