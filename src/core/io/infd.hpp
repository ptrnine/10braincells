#pragma once

#include <chrono>
#include <cstring>

#include <core/io/impl.hpp>

namespace core {
template <typename DerivedT, typename Impl, typename T = char, size_t S = 8192, auto... Settings>
class infd_base_t : public Impl { // NOLINT
public:
    template <typename DerivedT2, typename Impl2, typename TT, size_t SS, auto... SSettings>
    friend class infd_base_t;

    using fd_type = decltype(Impl::impl_open("", fd_flag{}, file_permissions{}));

    static constexpr bool exception_on_seek_fail = details::check_enable<fd_exception_on_seek_fail>(Settings...);
    static constexpr bool exception_on_read_fail = details::check_enable<fd_exception_on_read_fail>(Settings...);

    constexpr static DerivedT raw_create(fd_type fd) {
        DerivedT infd;
        infd.fd = fd;
        return infd;
    }

    infd_base_t() = default;

    infd_base_t(const char* filename, fd_combined_flag flags = fd_flag::read_only):
        fd(Impl::impl_open(filename, flags, file_permissions{})) {}

    infd_base_t(const std::string& filename, fd_combined_flag flags = fd_flag::read_only):
        infd_base_t(filename.data(), flags) {}

    infd_base_t(infd_base_t&&) noexcept = default;
    infd_base_t& operator=(infd_base_t&&) noexcept = default;

    ~infd_base_t() {
        Impl::impl_close(fd);
    }

    io_read_res read(T* destination, size_t count) noexcept(!exception_on_read_fail) {
        if (count > S) {
            auto wrote_size                = flush_to_dst(destination);
            auto reminder_size             = count - wrote_size;
            auto [wrote_reminder_size, rc] = Impl::impl_read(fd, destination + wrote_size, reminder_size * sizeof(T));
            auto actually_read             = wrote_size + wrote_reminder_size;
            return {actually_read, rc};
        }

        auto rc = io_read_rc::ok;
        if (size == 0)
            rc = take_next();

        if (rc != io_read_rc::ok && rc != io_read_rc::partial)
            return {0, rc};

        if (count <= size) {
            ::memcpy(destination, buf + pos, count * sizeof(T));
            size -= count;
            pos  += count;
            return {count, io_read_rc::ok};
        }
        else {
            auto actually_read = flush_to_dst(destination);
            return {actually_read, io_read_rc::partial};
        }
    }

    io_read_res nonblock_read(T* destination, size_t count) {
        if (blocking && can_be_blocked()) {
            if (Impl::impl_waitdev(fd, wait_timeout.count()))
                return read(destination, count);
            else
                return {0, io_read_rc::timeout};
        }
        else
            return read(destination, count);
    }

    bool is_blocking() const noexcept {
        return blocking;
    }

    auto read_wait_timeout() const noexcept {
        return wait_timeout;
    }

    void read_wait_timeout(auto duration) noexcept {
        wait_timeout = duration;
    }

    bool can_be_blocked() const {
        if (!stat_executed) {
            can_blocked   = Impl::impl_is_blocking(fd);
            stat_executed = true;
        }
        return can_blocked;
    }

    const fd_type& descriptor() const {
        return fd;
    }

private:
    io_read_rc take_next() {
        auto [actually_read, rc] = Impl::impl_read(fd, buf, S * sizeof(T));
        size                     = actually_read;
        pos                      = 0;
        return rc;
    }

    size_t flush_to_dst(T* dst) noexcept {
        if (size != 0) {
            size_t actually_read = size;
            ::memcpy(dst, buf + pos, size * sizeof(T));
            size = 0;
            pos  = 0;
            return actually_read;
        }
        return 0;
    }

private:
    fd_type                   fd   = Impl::default_infd();
    size_t                    size = 0;
    size_t                    pos  = 0;
    T                         buf[S];
    std::chrono::microseconds wait_timeout{0};
    bool                      blocking      = true;
    mutable bool              can_blocked   = false;
    mutable bool              stat_executed = false;
};

#define DECLARE_INFD(NAME, IMPL)                                                                                       \
    template <typename T = char, size_t S = 8192, auto... Settings>                                                    \
    struct NAME                                                                                                        \
        : public infd_base_t<NAME<T, S, Settings...>, IMPL<NAME<T, S, Settings...>, Settings...>, T, S, Settings...> { \
        using infd_base_t<NAME<T, S, Settings...>, IMPL<NAME<T, S, Settings...>, Settings...>, T, S, Settings...>::    \
            infd_base_t;                                                                                               \
    }

DECLARE_INFD(infd, outfd_linux_impl_t);
} // namespace core
