#pragma once

#include <sys/poll.hpp>
#include <core/traits/remove_cvref.hpp>

#define fwd(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace core
{
template <auto Tag, typename Handler = void>
struct poll_handle {
    constexpr poll_handle(int_const<Tag>, sys::fd_t ifd, sys::poll_events ievents, Handler ihandler):
        fd(ifd), events(ievents), handler(mov(ihandler)) {}

    constexpr poll_handle(sys::fd_t ifd, sys::poll_events ievents, Handler ihandler):
        fd(ifd), events(ievents), handler(mov(ihandler)) {}

    sys::fd_t        fd;
    sys::poll_events events;
    Handler          handler;
};

template <auto Tag>
struct poll_handle<Tag, void> {
    constexpr poll_handle(int_const<Tag>, sys::fd_t ifd, sys::poll_events ievents): fd(ifd), events(ievents) {}

    constexpr poll_handle(sys::fd_t ifd, sys::poll_events ievents): fd(ifd), events(ievents) {}

    sys::fd_t fd;
    sys::poll_events events;
};

static inline constexpr int_const<size_t(-1)> poll_empty_tag{};

template <auto Tag>
poll_handle(int_const<Tag>, sys::fd_t, sys::poll_events) -> poll_handle<Tag>;

poll_handle(sys::fd_t, sys::poll_events) -> poll_handle<poll_empty_tag>;

template <auto Tag, typename Handler>
poll_handle(int_const<Tag>, sys::fd_t, sys::poll_events, Handler) -> poll_handle<Tag, Handler>;

template <typename Handler>
poll_handle(sys::fd_t, sys::poll_events, Handler) -> poll_handle<poll_empty_tag, Handler>;

namespace poll_details
{
    template <typename T>
    struct poll_handle_traits {
        static inline constexpr bool value = false;
    };

    template <auto Tag, typename H>
    struct poll_handle_traits<poll_handle<Tag, H>> {
        static inline constexpr bool value = true;
        static inline constexpr auto tag   = Tag.to_int();
        using handler_type                 = H;
    };

    enum class disable_cast {};
    struct poll_empty_handler {
        constexpr bool operator()(disable_cast) const {
            return false;
        }
    };

    template <size_t DefaultTag, typename T>
    constexpr size_t current_tag(type_t<T>) {
        if constexpr (!poll_handle_traits<T>::value)
            return DefaultTag;
        else if constexpr (poll_handle_traits<T>::tag != poll_empty_tag)
            return poll_handle_traits<T>::tag;
        else
            return DefaultTag;
    }

    template <size_t PrevTag, typename T>
    constexpr size_t next_tag(type_t<T>) {
        if constexpr (!poll_handle_traits<T>::value)
            return PrevTag;
        else if constexpr (poll_handle_traits<T>::tag != poll_empty_tag)
            return poll_handle_traits<T>::tag + 1;
        else
            return PrevTag + 1;
    }

    template <size_t DefaultTag>
    constexpr auto make_handler(auto&& combined, auto&& arg) {
        using t = remove_cvref<decltype(arg)>;
        if constexpr (poll_handle_traits<t>::value) {
            if constexpr (is_same<typename poll_handle_traits<t>::handler_type, void>)
                return fwd(combined);
            else
                return overloaded{
                    fwd(combined),
                    [f = fwd(arg).handler](int_const<current_tag<DefaultTag>(type<remove_cvref<decltype(arg)>>)>,
                                           const sys::pollfd& pfd) mutable { return f(pfd); },
                };
        }
        else {
            return overloaded{fwd(combined), fwd(arg)};
        }
    }

    template <size_t Tag, size_t N>
    constexpr auto make_handlers_seq(auto&& combined) {
        return fwd(combined);
    }

    template <size_t Tag, size_t N>
    constexpr auto make_handlers_seq(auto&& combined, auto&& arg, auto&&... args) {
        constexpr auto new_tag = next_tag<Tag>(type<remove_cvref<decltype(arg)>>);
        return make_handlers_seq<new_tag, N + 1>(make_handler<Tag>(fwd(combined), fwd(arg)), fwd(args)...);
    }

    constexpr auto make_handlers(auto&&... args) {
        return make_handlers_seq<0, 0>(poll_empty_handler{}, fwd(args)...);
    }

    template <typename... Ts>
    constexpr size_t count_pfds(type_t<Ts>...) {
        return type_list<Ts...>.reduce(0, [](auto res, auto type) {
            if constexpr (poll_handle_traits<decl_type<type()>>::value)
                return res + 1;
            else
                return res;
        });
    }

    template <size_t I, size_t S>
    void set_pfd(array<sys::pollfd, S>& fds, auto&& arg, auto&&... args) {
        using t = remove_cvref<decltype(arg)>;
        if constexpr (poll_handle_traits<t>::value) {
            constexpr auto tag = current_tag<I>(type<t>);
            static_assert(tag < S);
            fds[tag].fd      = arg.fd;
            fds[tag].events  = arg.events;
            fds[tag].revents = sys::poll_event::none;

            if constexpr (sizeof...(args))
                return set_pfd<next_tag<I>(type<t>), S>(fds, fwd(args)...);
        }
        if constexpr (sizeof...(args))
            return set_pfd<I, S>(fds, fwd(args)...);
    }
} // namespace poll_details

template <size_t S, typename Handler>
class poller {
public:
    constexpr poller(auto&&... args): _handler(poll_details::make_handlers(fwd(args)...)) {
        poll_details::set_pfd<0>(_fds, fwd(args)...);
    }

    void run(opt<sys::nanoseconds> timeout = null) {
        while (true) {
            auto count   = poll(_fds, timeout).get();
            bool running = true;
            for (size_t i = 0; count > 0 && i < S; ++i) {
                if (_fds[i].revents & _fds[i].events) {
                    --count;
                    running =
                        running && idx_dispatch_r<bool, S>(i, [this](auto idx) {
                            if constexpr (is_same<invoke_result<remove_cvref<decltype(_handler)>, decltype(idx), decltype(_fds[idx])>,
                                                  void>) {
                                _handler(idx, _fds[idx]);
                                return true;
                            }
                            else
                                return _handler(idx, _fds[idx]);
                        });
                }
            }
            if (!running)
                break;
        }
    }

    void operator()(opt<sys::nanoseconds> timeout = null) {
        run(timeout);
    }

private:
    array<sys::pollfd, S> _fds;
    Handler               _handler;
};

poller(auto&&... args) -> poller<poll_details::count_pfds(type<remove_cvref<decltype(args)>>...),
                                 decltype(poll_details::make_handlers(fwd(args)...))>;

} // namespace core
