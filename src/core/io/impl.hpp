#pragma once

#include <sys/stat.h>
#include <unistd.h>

#include <iterator>

#include <core/errc.hpp>
#include <core/io/exceptions.hpp>
#include <core/io/flags.hpp>
#include <core/io/helpers.hpp>
#include <core/io/types.hpp>

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

    static int impl_open(const char* filename, fd_combined_flag flags, file_permissions create_permissions) {
        int fd;
        if ((fd = ::open(filename, flags.f, create_permissions.to_int())) < 0)
            throw cannot_open_file(filename, errc::from_errno());
        return fd;
    }

    static void impl_close(int fd) noexcept {
        if (fd != STDOUT_FILENO && fd != STDERR_FILENO && fd != STDIN_FILENO)
            ::close(fd);
    }

    static void impl_write(int fd, const void* data, size_t size) noexcept(!exception_on_syswrite_fail) {
        [[maybe_unused]] auto sz = ::write(fd, data, size);
        if constexpr (exception_on_syswrite_fail) {
            if (size_t(sz) != size)
                throw sys_write_fail(std::to_string(sz) + " of " + std::to_string(size) + " bytes has been wrote", {0});
        }
    }

    static io_read_res impl_read(int fd, void* data, size_t size) noexcept(!exception_on_read_fail) {
        auto actually_read = ::read(fd, data, size);
        if (actually_read < 0) {
            auto err = errc::from_errno();
            if (err == EAGAIN)
                return {0, io_read_rc::timeout};
            else if constexpr (exception_on_read_fail)
                throw sys_read_fail(err);
            else
                return {0, io_read_rc::error};
        }
        return {size_t(actually_read), actually_read == size ? io_read_rc::ok : io_read_rc::partial};
    }

    static void impl_seek(int fd, ssize_t value) noexcept(!exception_on_seek_fail) {
        [[maybe_unused]] auto rc = ::lseek(fd, value, SEEK_CUR);
        if constexpr (exception_on_seek_fail)
            if (rc < 0)
                throw sys_seek_fail(errc::from_errno());
    }

    constexpr static DerivedT stdout() {
        return DerivedT::raw_create(STDOUT_FILENO);
    }

    constexpr static DerivedT stderr() {
        return DerivedT::raw_create(STDERR_FILENO);
    }

    constexpr static DerivedT stdin() {
        return DerivedT::raw_create(STDIN_FILENO);
    }

    constexpr int default_outfd() const noexcept {
        return STDOUT_FILENO;
    }

    constexpr int default_infd() const noexcept {
        return STDIN_FILENO;
    }

    static bool impl_is_blocking(int fd) {
        struct stat st;
        auto rc = fstat(fd, &st);
        if (rc == -1)
            throw stat_fd_failed("", errc::from_errno());

        auto fmt = st.st_mode & S_IFMT;
        return fmt == S_IFSOCK || fmt == S_IFIFO || fmt == S_IFCHR;
    }

    static bool impl_waitdev(int fd, int64_t usec_timeout) {
        struct timeval tv {};
        tv.tv_usec = usec_timeout;

        fd_set readfds = {};
        FD_SET(fd, &readfds);
        return ::select(1, &readfds, nullptr, nullptr, &tv);
    }
};

template <typename DerivedT, auto... Settings>
struct outfd_devnull_impl {
    static constexpr bool exception_on_syswrite_fail =
        details::check_enable<fd_exception_on_syswrite_fail>(Settings...);
    static constexpr bool exception_on_seek_fail = details::check_enable<fd_exception_on_seek_fail>(Settings...);

    static int impl_open(const char*, fd_combined_flag, file_permissions) {
        return 0;
    }
    static void impl_close(int) {}
    static void impl_write(int, const void*, size_t) noexcept(!exception_on_syswrite_fail) {}
    static io_read_res impl_read(int, void*, size_t) noexcept {
        return {0};
    }
    static void impl_seek(int, ssize_t) noexcept(!exception_on_seek_fail) {}

    constexpr int default_outfd() const noexcept {
        return 0;
    }

    constexpr int default_infd() const noexcept {
        return 0;
    }

    constexpr static bool impl_is_blocking(int) {
        return false;
    }

    static bool impl_waitdev(int, int64_t) {
        return true;
    }
};
} // namespace core
