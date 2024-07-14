#pragma once

#include <iterator>

#include <core/errc.hpp>
#include <core/io/exceptions.hpp>
#include <core/io/helpers.hpp>
#include <core/io/types.hpp>

#include <sys/close.hpp>
#include <sys/open.hpp>
#include <sys/read.hpp>
#include <sys/statx.hpp>
#include <sys/write.hpp>
#include <sys/lseek.hpp>
#include <sys/poll.hpp>

namespace core {
namespace outfd_concepts
{
    using std::begin, std::end;

    template <typename StringLike, typename T>
    concept WritableBase = requires(const StringLike& s) {
                               { begin(s) } -> std::convertible_to<const T*>;
                               { end(s) } -> std::convertible_to<const T*>;
                               { end(s) - begin(s) } -> std::convertible_to<ptrdiff_t>;
                           };

    template <typename Container, typename T>
    concept WritableRAI = requires(const Container& s) {
                              { begin(s).base() } -> std::convertible_to<const T*>;
                              { *end(s) } -> std::convertible_to<T>;
                              { end(s) - begin(s) } -> std::convertible_to<ptrdiff_t>;
                          };

    template <typename Container, typename T>
    concept Writable = WritableBase<Container, T> || WritableRAI<Container, T> || std::convertible_to<Container, T>;
} // namespace outfd_concepts

template <typename DerivedT, auto... Settings>
struct outfd_linux_impl_t {
    static constexpr bool exception_on_syswrite_fail =
        details::check_enable<fd_exception_on_syswrite_fail>(Settings...);
    static constexpr bool exception_on_seek_fail    = details::check_enable<fd_exception_on_seek_fail>(Settings...);
    static constexpr bool exception_on_read_fail    = details::check_enable<fd_exception_on_read_fail>(Settings...);

    static sys::fd_t impl_open(const char* filename, sys::openflags flags, sys::file_perms create_permissions) {
        if (auto res = sys::open(filename, flags, create_permissions))
            return *res;
        else
            throw cannot_open_file(filename, res.unsafe_error());
    }

    static void impl_close(sys::fd_t fd) noexcept {
        if (fd != sys::stdin_fd && fd != sys::stdout_fd && fd != sys::stderr_fd)
            sys::close(fd);
    }

    static void impl_write(sys::fd_t fd, const void* data, size_t size) noexcept(!exception_on_syswrite_fail) {
        [[maybe_unused]] auto res = sys::write(fd, data, size);
        if constexpr (exception_on_syswrite_fail) {
            if (!res)
                throw sys_write_fail("", res.unsafe_error());
            else if (*res < size)
                throw sys_write_fail(
                    "Only " + std::to_string(*res) + " of " + std::to_string(size) + " bytes has been wrote", {0});
        }
    }

    static io_read_res impl_read(sys::fd_t fd, void* data, size_t size) noexcept(!exception_on_read_fail) {
        auto res = sys::read(fd, data, size);
        if (!res) {
            if (res.unsafe_error() == EAGAIN)
                return {0, io_read_rc::timeout};
            else if constexpr (exception_on_read_fail)
                throw sys_read_fail(res.unsafe_error());
            else
                return {0, io_read_rc::error};
        }
        return {*res, *res == size ? io_read_rc::ok : io_read_rc::partial};
    }

    static void impl_seek(sys::fd_t fd, ssize_t value) noexcept(!exception_on_seek_fail) {
        [[maybe_unused]] auto rc = sys::lseek(fd, value, sys::seek_whence::cur);
        if constexpr (exception_on_seek_fail)
            if (!rc)
                throw sys_seek_fail(rc.unsafe_error());
    }

    constexpr static DerivedT stdout() {
        return DerivedT{sys::stdout_fd};
    }

    constexpr static DerivedT stderr() {
        return DerivedT{sys::stderr_fd};
    }

    constexpr static DerivedT stdin() {
        return DerivedT{sys::stdin_fd};
    }

    [[nodiscard]]
    static constexpr sys::fd_t invalid_fd() noexcept {
        return sys::invalid_fd;
    }

    static bool impl_is_blocking(sys::fd_t fd) {
        auto res = sys::statx(fd, sys::statx_mask::mode);
        if (!res)
            throw stat_fd_failed("", res.unsafe_error());

        switch (res->mode.type()) {
            case sys::file_type::socket:
            case sys::file_type::fifo:
            case sys::file_type::chardev:
                return true;
            default:
                return false;
        }
    }

    static bool impl_waitdev(sys::fd_t fd, sys::nanoseconds timeout) {
        return sys::poll(sys::pollfd{fd, sys::poll_event::in, {}}, timeout).ok();
    }
};

template <typename DerivedT, auto... Settings>
struct outfd_devnull_impl {
    static constexpr bool exception_on_syswrite_fail =
        details::check_enable<fd_exception_on_syswrite_fail>(Settings...);
    static constexpr bool exception_on_seek_fail = details::check_enable<fd_exception_on_seek_fail>(Settings...);

    static sys::fd_t impl_open(const char*, sys::openflags, sys::file_perms) {
        return sys::stdout_fd;
    }
    static void impl_close(sys::fd_t) {}
    static void impl_write(sys::fd_t, const void*, size_t) noexcept(!exception_on_syswrite_fail) {}
    static io_read_res impl_read(sys::fd_t, void*, size_t) noexcept {
        return {0};
    }
    static void impl_seek(sys::fd_t, ssize_t) noexcept(!exception_on_seek_fail) {}

    [[nodiscard]]
    constexpr sys::fd_t default_outfd() const noexcept {
        return sys::stdout_fd;
    }

    [[nodiscard]]
    static constexpr sys::fd_t invalid_fd() {
        return sys::invalid_fd;
    }

    constexpr static bool impl_is_blocking(sys::fd_t) {
        return false;
    }

    static bool impl_waitdev(sys::fd_t, sys::nanoseconds) {
        return true;
    }
};
} // namespace core
