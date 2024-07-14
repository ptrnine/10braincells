#pragma once

#include <mutex>
#include <thread>

#include <core/function.hpp>
#include <core/moveonly_trivial.hpp>
#include <core/traits/remove_cvref.hpp>
#include <sys/close.hpp>
#include <sys/epoll.hpp>
#include <sys/open_flags.hpp>

#define fwd(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace core
{

class epoller_pack_fd_ptr_error : exception {
public:
    epoller_pack_fd_ptr_error(int fd):
        msg("Cannot pack fd value " + std::to_string(fd) + " because it greater than 32767") {}

    const char* what() const noexcept override {
        return msg.data();
    }

private:
    std::string msg;
};

namespace details
{
    namespace epoll_userdata_bits
    {
        enum bits : u64 {
            handler = u64(1) << 63,
        };
    }

    u64 epoll_pack_fd_ptr(sys::fd_t fd, void* ptr) {
        constexpr u64 max_fd = (~u64(0) >> 49);
        if (u64(fd) > max_fd)
            throw epoller_pack_fd_ptr_error(int(fd));

        auto fd_mask = u64(fd) << 48;
        return (u64)ptr | fd_mask | epoll_userdata_bits::handler;
    }

    bool epoll_have_handler(const sys::epoll_data& data) {
        return u64(data.data.u64) & epoll_userdata_bits::handler;
    }

    auto epoll_unpack_fd_ptr(const sys::epoll_data& data) {
        auto packed = u64(data.data.u64) & (~epoll_userdata_bits::handler);
        return tuple{sys::fd_t(packed >> 48), (void*)(packed & (~u64(0) >> 16))};
    }
} // namespace details

class epoller_invalid_group : exception {
public:
    epoller_invalid_group(u32 group): msg("Invalid group number " + std::to_string(group)) {}

    const char* what() const noexcept override {
        return msg.data();
    }

private:
    std::string msg;
};

namespace details {
    template <size_t MaxGroups, auto Value>
    auto check_group_type(int_const<Value>) {
        static_assert(size_t(Value) < MaxGroups, "Invalid group");
        return Value;
    }

    template <size_t MaxGroups>
    auto check_group_type(auto value) {
        if (size_t(value) >= MaxGroups)
            throw epoller_invalid_group(u32(value));
        return value;
    }

    template <typename IdType>
    auto wrap_handler_return(auto&& handler) {
        using result = invoke_result<remove_cvref<decltype(handler)>, IdType, sys::fd_t, sys::epoll_events>;
        if constexpr (is_same<result, void>)
            return [h = fwd(handler)](IdType id, sys::fd_t fd, sys::epoll_events events) mutable {
                h(id, fd, events);
                return true;
            };
        else
            return fwd(handler);
    }

    template <typename IdType>
    auto wrap_handler_types(auto&& handler) {
        if constexpr (requires { handler(IdType{}, sys::fd_t{}, sys::epoll_events{}); })
            return wrap_handler_return<IdType>(fwd(handler));
        else
            return wrap_handler_return<IdType>(
                [h = fwd(handler)](IdType, sys::fd_t fd, sys::epoll_events events) mutable { return h(fd, events); });
    }
} // namespace details

template <typename GroupType = type_t<u8>, typename IdType = type_t<u32>>
    requires(sizeof(typename IdType::type) <= 4)
struct epoller_cfg {
    size_t    max_groups       = 8;
    GroupType group_type       = {};
    IdType    id_type          = {};
    size_t    max_handler_size = 8;
};

/* XXX: 8 bits for group 24 bits for id */
template <auto Cfg = epoller_cfg{}> requires(Cfg.max_groups <= 0xff)
class epoller {
public:
    static inline constexpr auto max_groups       = Cfg.max_groups;
    static inline constexpr auto max_handler_size = Cfg.max_handler_size;
    using group_type                              = decl_type<Cfg.group_type>;
    using id_type                                 = decl_type<Cfg.id_type>;
    static inline constexpr auto default_group    = group_type{0};
    static inline constexpr auto default_id       = id_type{0};

    using fd_type             = moveonly_trivial<sys::fd_t, sys::invalid_fd>;
    using dedicated_handler_t = bool(sys::fd_t, sys::epoll_events);
    using handler_t           = bool(id_type, sys::fd_t, sys::epoll_events);

    epoller(sys::epoll_flags flags = sys::epoll_flag::none): epfd(sys::epoll_create(flags).get()) {}

    epoller(epoller&&) noexcept            = default;
    epoller& operator=(epoller&&) noexcept = default;

    ~epoller() {
        if (epfd != sys::invalid_fd)
            sys::close(epfd).throw_if_error();
    }

    void add_with_dedicated_handler(sys::fd_t fd, sys::epoll_events events, dedicated_handler_t handler) {
        sys::epoll_ctl_add(epfd, fd, events, details::epoll_pack_fd_ptr(fd, (void*)handler)).throw_if_error();
    }

    void
    add_with_dedicated_handler(std::span<const sys::fd_t> fds, sys::epoll_events events, dedicated_handler_t handler) {
        for (auto fd : fds)
            add_with_dedicated_handler(fd, events, handler);
    }

    template <typename T = int_const<default_group>>
    void add(sys::fd_t fd, sys::epoll_events events, T group = int_c<default_group>, id_type id = default_id) {
        auto grp = details::check_group_type<max_groups>(group);
        auto userdata = u64(fd) | (u64(grp) << 32) | (u64(id) << 40);
        sys::epoll_ctl_add(epfd, fd, events, userdata).throw_if_error();
    }

    template <typename T = int_const<default_group>>
    void add(std::span<const sys::fd_t> fds, sys::epoll_events events, T group = int_c<default_group>) {
        for (auto fd : fds)
            add(fd, events, group);
    }

    template <typename GroupT>
    void add_handler(GroupT group, auto&& handler) {
        auto grp = details::check_group_type<max_groups>(group);
        std::lock_guard lock{_mtx};
        _handlers[u32(grp)] = details::wrap_handler_types<id_type>(fwd(handler));
    }

    template <typename GroupT>
    void remove_handler(GroupT group) {
        auto grp = details::check_group_type<max_groups>(group);
        std::lock_guard lock{_mtx};
        _handlers[u32(grp)] = null;
    }

    bool remove(sys::fd_t fd) {
        auto res = sys::epoll_ctl_del(epfd, fd);
        if (res.error() == errc::enoent)
            return false;
        res.throw_if_error();
        return true;
    }

    void wait(std::span<sys::epoll_data> buffer,
              opt<sys::nanoseconds>      timeout = null,
              sys::sigset                sigmask = sys::sigset::empty()) {
        std::unique_lock lock{_mtx};
        auto             handlers = _handlers;
        lock.unlock();

        auto size = sys::epoll_wait(epfd, buffer, timeout, sigmask).get();
        for (size_t i = 0; i < size; ++i) {
            const auto& data = buffer[i];

            if (details::epoll_have_handler(data)) {
                auto [fd, handler_ptr] = details::epoll_unpack_fd_ptr(data);
                auto handler           = (dedicated_handler_t*)handler_ptr;
                auto continue_watch    = handler(fd, data.events);
                if (!continue_watch)
                    remove(fd);
            }
            else {
                /* XXX: LE only */
                auto fd    = data.data.fd;
                auto group = u32(data.data.u64 >> 32) & 0xff;
                if (group > max_groups)
                    throw epoller_invalid_group(group);

                auto id = u32(data.data.u64 >> 40);

                if (handlers[group]) {
                    auto continue_watch = handlers[group](id_type(id), fd, data.events);
                    if (!continue_watch)
                        remove(fd);
                }
                else {
                    remove(fd);
                }
            }
        }
    }

    template <size_t BuffSize = 8>
    std::jthread launch(opt<sys::nanoseconds> timeout = null, sys::sigset sigmask = sys::sigset::empty()) {
        return std::jthread{[this, timeout, sigmask](std::stop_token st) {
            sys::epoll_data buff[8];
            while (!st.stop_requested())
                this->wait(buff, timeout, sigmask);
        }};
    }

private:
    fd_type                                                  epfd;
    array<function<handler_t, max_handler_size>, max_groups> _handlers;
    mutable std::mutex                                       _mtx;
};
} // namespace core

#undef fwd
