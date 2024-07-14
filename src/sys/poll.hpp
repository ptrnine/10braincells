#pragma once

#include <span>
#include <chrono>

#include <csignal>

#include <core/array.hpp>
#include <core/opt.hpp>
#include <core/tuple.hpp>
#include <sys/basic_types.hpp>
#include <sys/chrono.hpp>
#include <sys/syscall.hpp>

namespace sys {
/*
 * [[[codegen start]]]
 * Generated with:
 * ./build-deb/codegen/flags poll_event poll_events short none=0 in=0x1 pri=0x2 out=0x4 error=0x8 hungup=0x10 invalid=0x20
 */

enum class poll_event : short {
    none = 0,
    in = 0x1,
    pri = 0x2,
    out = 0x4,
    error = 0x8,
    hungup = 0x10,
    invalid = 0x20,
};

struct poll_events {
    constexpr poll_events() = default;
    constexpr poll_events(poll_event ivalue): value(short(ivalue)) {}

    constexpr poll_events operator|(poll_events flag) const {
        return poll_event(value | flag.value);
    }

    constexpr poll_events operator&(poll_events flag) const {
        return poll_event(value & flag.value);
    }

    constexpr poll_events& operator|=(poll_events flag) {
        value = short(value | flag.value);
        return *this;
    }

    constexpr bool test(poll_events flags) const {
        return value & flags.value;
    }

    explicit constexpr operator bool() const {
        return value;
    }

    constexpr std::string to_string() const {
        constexpr core::array flags = {
            core::tuple{poll_event::none, std::string_view("none")},
            core::tuple{poll_event::in, std::string_view("in")},
            core::tuple{poll_event::pri, std::string_view("pri")},
            core::tuple{poll_event::out, std::string_view("out")},
            core::tuple{poll_event::error, std::string_view("error")},
            core::tuple{poll_event::hungup, std::string_view("hungup")},
            core::tuple{poll_event::invalid, std::string_view("invalid")},
        };
        std::string res;
        for (auto&& [f, s] : flags) {
            if (short(value) & short(f)) {
                res.append(s);
                res.append(" | ");
            }
        }
        if (res.size() > 2)
            res.resize(res.size() - 2);
        return res;
    }

    short value;
};

inline constexpr poll_events operator|(poll_event lhs, poll_event rhs) {
    return poll_event(short(lhs) | short(rhs));
}

/* [[[codegen end]]] */

struct pollfd {
    fd_t        fd;      /* File descriptor to poll.  */
    poll_events events;  /* Types of events poller cares about.  */
    poll_events revents; /* Types of events that actually occurred.  */
};

inline auto poll(std::span<const pollfd> fds, core::opt<nanoseconds> timeout = core::null) {
    if (timeout) {
        long t[2];
        t[0] = duration_cast<seconds>(*timeout).count();
        t[1] = (*timeout - seconds{t[0]}).count();
        return syscall<size_t, trap_async_cancel>(SYS_ppoll, fds.data(), fds.size(), t, 0, _NSIG / 8);
    }
    else
        return syscall<size_t, trap_async_cancel>(SYS_ppoll, fds.data(), fds.size(), 0, 0, _NSIG / 8);

}

inline auto poll(const pollfd& fd, core::opt<nanoseconds> timeout = core::null) {
    return poll(std::span{&fd, 1}, timeout);
}
} // namespace sys
