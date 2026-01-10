#pragma once

#include <core/moveonly_trivial.hpp>
#include <sys/close.hpp>
#include <sys/memfd_create.hpp>
#include <sys/memfd_secret.hpp>
#include <sys/open.hpp>
#include <sys/pidfd_open.hpp>
#include <sys/pipe.hpp>

#include <core/io/basic_types.hpp>

#define fwd(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace core::io
{
class file {
public:
    static constexpr file memfd(auto&&... args) {
        return {sys::memfd_create(fwd(args)...).get()};
    }

    static constexpr file memfd_secret(bool close_exec = false) {
        return {sys::memfd_secret(close_exec).get()};
    }

    static constexpr file open(auto&&... args) {
        return {sys::open(fwd(args)...).get()};
    }

    static constexpr file pidfd(sys::pid_t pid, sys::pidfd_open_flags flags = {}) {
        return {sys::pidfd_open(pid, flags).get()};
    }

    static constexpr auto pipe(pipeflags flags = {});

    constexpr file() = default;

    constexpr ~file() {
        if (_fd.not_default())
            sys::close(_fd);
    }

    constexpr file(file&&) noexcept = default;

    constexpr file& operator=(file&& f) noexcept {
        if (_fd.not_default())
            sys::close(_fd);
        _fd = mov(f._fd);
        return *this;
    }

    constexpr operator sys::fd_t() const {
        return _fd;
    }

    constexpr sys::fd_t fd() const {
        return _fd;
    }

    explicit operator bool() const {
        return valid();
    }

    bool valid() const {
        return _fd.not_default();
    }

private:
    constexpr file(sys::fd_t ifd): _fd(ifd) {}

    moveonly_trivial<sys::fd_t, sys::invalid_fd> _fd;
};

struct file_pipe_result {
    file in;
    file out;

    operator sys::pipe_result() const {
        return {.in = in, .out = out};
    }
};

constexpr auto file::pipe(pipeflags flags) {
    auto [i, o] = sys::pipe(flags).get();
    return file_pipe_result{.in = i, .out = o};
}
} // namespace core::io

#undef fwd
