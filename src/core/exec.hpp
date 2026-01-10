#pragma once

#include <sys/dup.hpp>
#include <sys/execve.hpp>
#include <sys/fork.hpp>
#include <sys/open_flags.hpp>
#include <sys/pidfd_open.hpp>
#include <sys/pipe.hpp>
#include <sys/waitid.hpp>

#include <core/io/file.hpp>
#include <core/io/in.hpp>
#include <core/io/out.hpp>
#include <core/opt.hpp>
#include <core/poller.hpp>
#include <core/ranges/range.hpp>
#include <core/ranges/to.hpp>
#include <core/ranges/zip.hpp>
#include <core/epoller.hpp>

#define fwd(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace core {
class process {
public:
    process(auto&&... args) {
        (args.visit(*this), ...);
    }

    process& set_args(std::vector<std::string> args) {
        _args_holder.clear();
        _args.clear();
        _args_holder.reserve(args.size());
        _args.reserve(args.size() + 1);

        for (auto&& a : args) {
            _args_holder.push_back(mov(a));
            _args.push_back(_args_holder.back().data());
        }
        _args.push_back(nullptr);

        return *this;
    }

    process& set_env(std::vector<std::string> env) {
        _env_holder.clear();
        _env.clear();
        _env_holder.reserve(env.size());
        _env.reserve(env.size() + 1);

        for (auto&& a : env) {
            _env_holder.push_back(mov(a));
            _env.push_back(_env_holder.back().data());
        }
        _env.push_back(nullptr);

        return *this;
    }

    process& run() {
        opt<sys::fd_t> fds[3];
        for (auto&& [i, fd, store] : with_index(fds, pipes)) {
            setup_pipe(i, fd, store);
        }

        _pid = sys::fork().get();
        if (*_pid == 0) {
            for (auto&& [i, fd] : with_index(fds)) {
                setup_pipe_child(i, fd);
            }

            sys::close_range(3, limits<u32>::max());

            sys::execve(_args[0], _args.data(), _env.data()).throw_if_error();
            __builtin_unreachable();
        } else {
            //
        }

        return *this;
    }

    struct signal {
        constexpr auto operator<=>(const signal&) const noexcept = default;
        int num;
    };
    struct exit_code {
        constexpr auto operator<=>(const exit_code&) const noexcept = default;
        int num;
    };

    using result = var<exit_code, signal>;

    result wait() {
        if (!_pid) {
            run();
        }

        auto pfd = io::file::pidfd(_pid.value());

        epoller<epoller_cfg<>{.max_handler_size = 16}> ep;
        ep.add_with_dedicated_handler(pfd, sys::epoll_event::in, [](sys::fd_t, sys::epoll_events) { return epoller_handler_result::shutdown; });

        opt<decltype(io::in{declval<std::string&>()})> in;

        if (io[0].is_type(type<std::string*>)) {
            in = io::in{*io[0].get(type<std::string*>)};
        }

        if (io[1].is_type(type<std::string*>) || io[2].is_type(type<std::string*>)) {
            ep.add_handler(int_c<0>, [&](u32 id, sys::fd_t fd, sys::epoll_events) {
                if (id == 0) {
                    (*in) >> io::out{fd};
                } else {
                    io::in{fd} >> io::out{*io[id].get(type<std::string*>)};
                }
            });
        }

        for (size_t i = 0; i < 3; ++i) {
            if (io[i].is_type(type<std::string*>)) {
                ep.add(i == 0 ? pipes[i]->out : pipes[i]->in, i == 0 ? sys::epoll_event::out : sys::epoll_event::in, int_c<0>, u32(i));
            }
        }

        sys::epoll_data buff[4];
        while (ep.wait(buff)) {}

        _pid.reset();

        auto info = sys::waitid(sys::wait_type::pidfd, pfd, sys::wait_flag::exited).get();
        if (info.is_exited()) {
            return exit_code{info.status()};
        }
        return signal{info.status()};
    }

    process& set_stdin(auto&& value) {
        set_std_fd(sys::stdin_fd, value);
        return *this;
    }

    process& set_stdout(auto&& value) {
        set_std_fd(sys::stdout_fd, value);
        return *this;
    }

    process& set_stderr(auto&& value) {
        set_std_fd(sys::stderr_fd, value);
        return *this;
    }

    opt<sys::pid_t> pid() const {
        return _pid;
    }

private:
    std::vector<std::string> _args_holder;
    std::vector<std::string> _env_holder;
    std::vector<const char*> _args;
    std::vector<const char*> _env;

    using io_info = var<sys::fd_t, std::string*, null_t>;

    io_info                   io[3] = {sys::stdin_fd, sys::stdout_fd, sys::stderr_fd};
    opt<io::file_pipe_result> pipes[3];
    opt<sys::pid_t>           _pid;

private:
    void set_std_fd(sys::fd_t std_fd, sys::fd_t fd) {
        if (fd != sys::invalid_fd) {
            io[size_t(std_fd)] = fd;
        } else {
            io[size_t(std_fd)] = null;
        }
    }

    void set_std_fd(sys::fd_t std_fd, std::string& str) {
        io[size_t(std_fd)] = &str;
    }

    void set_std_fd(sys::fd_t std_fd, std::nullptr_t) {
        io[size_t(std_fd)] = null;
    }

    void set_std_fd(sys::fd_t std_fd, sys::pipe_result pipe) {
        io[size_t(std_fd)] = std_fd == sys::stdin_fd ? pipe.in : pipe.out;
    }

    void setup_pipe(size_t i, opt<sys::fd_t>& fd, opt<io::file_pipe_result>& store) const {
        visit(
            io[i],
            overloaded{
                [&](sys::fd_t new_fd) { fd = new_fd; },
                [&](std::string*) {
                    store = io::file::pipe();
                    fd = (i == 0 ? store->in : store->out);
                },
                [](null_t) {}
            }
        );
    }

    void setup_pipe_child(size_t i, opt<sys::fd_t>& fd) const {
        if (fd) {
            sys::dup2(*fd, sys::fd_t(i)).throw_if_error();
        } else {
            sys::close(sys::fd_t(i));
        }
    }
};

namespace arg {

#define TBC_DEF_ARGS_SET(NAME, METHOD)                                    \
    constexpr struct _##NAME {                                            \
        /* NOLINTNEXTLINE */                                              \
        constexpr auto operator=(std::vector<std::string> values) const { \
            struct res {                                                  \
                void visit(process& p) {                                  \
                    p.METHOD(mov(vs));                                    \
                }                                                         \
                std::vector<std::string> vs;                              \
            };                                                            \
            return res{mov(values)};                                      \
        }                                                                 \
    } NAME

#define TBC_DEF_IO_SET(NAME, METHOD)                   \
    constexpr struct _##NAME {                         \
        /* NOLINTNEXTLINE */                           \
        constexpr auto operator=(auto&& value) const { \
            struct res {                               \
                void visit(process& p) {               \
                    p.METHOD(v);                       \
                }                                      \
                /* NOLINTNEXTLINE */                   \
                decltype(value)&& v;                   \
            };                                         \
            return res{value};                         \
        }                                              \
    } NAME

TBC_DEF_ARGS_SET(args, set_args);
TBC_DEF_ARGS_SET(env, set_env);
TBC_DEF_IO_SET(std_in, set_stdin);
TBC_DEF_IO_SET(std_out, set_stdout);
TBC_DEF_IO_SET(std_err, set_stderr);

#undef TBC_DEF_ARGS_SET
#undef TBC_DEF_IO_SET
} // namespace arg

struct exec_result {
    process::result code;
    std::string     std_out;
    std::string     std_err;
};

inline exec_result exec(std::vector<std::string> args, std::vector<std::string> env = {}) {
    std::string sout, serr;
    auto        code = process{arg::args = mov(args), arg::env = mov(env), arg::std_out = sout, arg::std_err = serr}.wait();
    return {
        .code    = mov(code),
        .std_out = mov(sout),
        .std_err = mov(serr),
    };
}
} // namespace core

#undef fwd
