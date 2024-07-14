#pragma once

#include <sys/epoll.h>

#include <core/array.hpp>
#include <core/opt.hpp>
#include <core/tuple.hpp>

#include <sys/chrono.hpp>
#include <sys/signal.hpp>
#include <sys/syscall.hpp>

namespace sys {
/*
 * [[[codegen start]]]
 * Generated with:
 * ./build-deb/codegen/flags epoll_flag epoll_flags int none=0 close_exec=EPOLL_CLOEXEC
 */
enum class epoll_flag : int {
    none       = 0,
    close_exec = EPOLL_CLOEXEC,
};

struct epoll_flags {
    using enum epoll_flag;

    constexpr epoll_flags() = default;
    constexpr epoll_flags(epoll_flag ivalue): value(int(ivalue)) {}

    constexpr epoll_flags operator|(epoll_flags flag) const {
        return epoll_flag(int(value | flag.value));
    }

    constexpr epoll_flags operator&(epoll_flags flag) const {
        return epoll_flag(int(value & flag.value));
    }

    constexpr epoll_flags& operator|=(epoll_flags flag) {
        value = int(value | flag.value);
        return *this;
    }

    constexpr bool test(epoll_flags flags) const {
        return value & flags.value;
    }

    explicit constexpr operator bool() const {
        return value;
    }

    constexpr void unset(epoll_flags flags) {
        value &= int(~flags.value);
    }

    constexpr std::string to_string() const {
        constexpr core::array flags = {
            core::tuple{epoll_flag::none, std::string_view("none")},
            core::tuple{epoll_flag::close_exec, std::string_view("close_exec")},
        };
        std::string res;
        for (auto&& [f, s] : flags) {
            if (int(value) & int(f)) {
                res.append(s);
                res.append(" | ");
            }
        }
        if (res.size() > 2)
            res.resize(res.size() - 2);
        return res;
    }

    int value;
};

inline constexpr epoll_flags operator|(epoll_flag lhs, epoll_flag rhs) {
    return epoll_flag(int(lhs) | int(rhs));
}
/* [[[codegen end]]] */

/*
 * [[[codegen start]]]
 * Generated with:
 * ./build-deb/codegen/flags epoll_event epoll_events u32 in=EPOLLIN out=EPOLLOUT pri=EPOLLPRI error=EPOLLERR
 * hungup=EPOLLHUP edge_triggered=EPOLLET oneshot=EPOLLONESHOT wakeup=EPOLLWAKEUP exclusive=EPOLLEXCLUSIVE
 */
enum class epoll_event : u32 {
    in             = EPOLLIN,
    out            = EPOLLOUT,
    pri            = EPOLLPRI,
    error          = EPOLLERR,
    hungup         = EPOLLHUP,
    edge_triggered = EPOLLET,
    oneshot        = EPOLLONESHOT,
    wakeup         = EPOLLWAKEUP,
    exclusive      = EPOLLEXCLUSIVE,
};

struct epoll_events {
    using enum epoll_event;

    constexpr epoll_events() = default;
    constexpr epoll_events(epoll_event ivalue): value(u32(ivalue)) {}

    constexpr epoll_events operator|(epoll_events flag) const {
        return epoll_event(u32(value | flag.value));
    }

    constexpr epoll_events operator&(epoll_events flag) const {
        return epoll_event(u32(value & flag.value));
    }

    constexpr epoll_events& operator|=(epoll_events flag) {
        value = u32(value | flag.value);
        return *this;
    }

    constexpr bool test(epoll_events flags) const {
        return value & flags.value;
    }

    explicit constexpr operator bool() const {
        return value;
    }

    constexpr void unset(epoll_events flags) {
        value &= u32(~flags.value);
    }

    constexpr std::string to_string() const {
        constexpr core::array flags = {
            core::tuple{epoll_event::in, std::string_view("in")},
            core::tuple{epoll_event::out, std::string_view("out")},
            core::tuple{epoll_event::pri, std::string_view("pri")},
            core::tuple{epoll_event::error, std::string_view("error")},
            core::tuple{epoll_event::hungup, std::string_view("hungup")},
            core::tuple{epoll_event::edge_triggered, std::string_view("edge_triggered")},
            core::tuple{epoll_event::oneshot, std::string_view("oneshot")},
            core::tuple{epoll_event::wakeup, std::string_view("wakeup")},
            core::tuple{epoll_event::exclusive, std::string_view("exclusive")},
        };
        std::string res;
        for (auto&& [f, s] : flags) {
            if (u32(value) & u32(f)) {
                res.append(s);
                res.append(" | ");
            }
        }
        if (res.size() > 2)
            res.resize(res.size() - 2);
        return res;
    }

    u32 value;
};

inline constexpr epoll_events operator|(epoll_event lhs, epoll_event rhs) {
    return epoll_event(u32(lhs) | u32(rhs));
}
/* [[[codegen end]]] */

template <typename T>
concept epoll_userdata = core::is_ptr<T> || core::any_of<T, fd_t, u32, u64>;

struct
#ifdef __x86_64__
    __attribute__((__packed__))
#endif
epoll_data {
    static inline constexpr epoll_data create(epoll_events events, epoll_userdata auto userdata) {
        epoll_data res = {events, {}};
        __builtin_memcpy(&res.data, &userdata, sizeof(userdata));
        return res;
    }

    epoll_events events;
    union {
        void* ptr;
        fd_t  fd;
        u32   u32;
        u64   u64;
    } data;
};

enum class epoll_ctl_op {
    add = EPOLL_CTL_ADD,
    mod = EPOLL_CTL_MOD,
    del = EPOLL_CTL_DEL,
};

inline auto epoll_create(epoll_flags flags = epoll_flag::none) {
    return syscall<fd_t>(SYS_epoll_create1, flags.value);
}

inline auto epoll_ctl(fd_t epfd, epoll_ctl_op op, fd_t fd, core::opt<epoll_data> events) {
    return syscall<void>(SYS_epoll_ctl, epfd, op, fd, events ? &(*events) : nullptr);
}

template <epoll_userdata T = void*>
inline auto epoll_ctl_add(fd_t epfd, fd_t fd, epoll_events events, T userdata = nullptr) {
    return epoll_ctl(epfd, epoll_ctl_op::add, fd, epoll_data::create(events, userdata));
}

template <epoll_userdata T = void*>
inline auto epoll_ctl_mod(fd_t epfd, fd_t fd, epoll_events events, T userdata = nullptr) {
    return epoll_ctl(epfd, epoll_ctl_op::mod, fd, epoll_data::create(events, userdata));
}

inline auto epoll_ctl_del(fd_t epfd, fd_t fd) {
    return epoll_ctl(epfd, epoll_ctl_op::del, fd, core::null);
}

inline auto epoll_wait(fd_t                   epfd,
                       std::span<epoll_data>  output,
                       core::opt<nanoseconds> timeout = core::null,
                       sigset                 sigmask = sigset::empty()) {
    if (timeout) {
        long t[2];
        t[0] = duration_cast<seconds>(*timeout).count();
        t[1] = (*timeout - seconds{t[0]}).count();
        return syscall<size_t, trap_async_cancel>(
            SYS_epoll_pwait2, epfd, output.data(), output.size(), t, &sigmask, _NSIG / 8);
    }
    return syscall<size_t, trap_async_cancel>(
        SYS_epoll_pwait2, epfd, output.data(), output.size(), nullptr, &sigmask, _NSIG / 8);
}

inline auto epoll_wait(fd_t                   epfd,
                       epoll_data&            output,
                       core::opt<nanoseconds> timeout = core::null,
                       sigset                 sigmask = sigset::empty()) {
    return epoll_wait(epfd, std::span{&output, 1}, timeout, sigmask);
}
} // namespace sys
