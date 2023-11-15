#pragma once

#include <chrono>
#include <cstring>

#include <core/io/impl.hpp>

namespace core {
template <typename T, typename Infd>
struct infd_scroll_range_base {
    [[nodiscard]]
    const T* begin() const {
        return _ifd->buf;
    }

    [[nodiscard]]
    const T* end() const {
        return _ifd->buf + _ifd->size;
    }

    [[nodiscard]]
    const T* data() const {
        return _ifd->buf;
    }

    [[nodiscard]]
    size_t size() const {
        return _ifd->size;
    }

    const Infd* _ifd;
};

template <typename T, typename Infd>
struct infd_scroll_range : infd_scroll_range_base<T, Infd> {};

template <typename Infd>
struct infd_scroll_range<char, Infd> : infd_scroll_range_base<char, Infd> {
    [[nodiscard]]
    std::string_view to_sv() const {
        return {this->data(), this->size()};
    }

    explicit operator std::string_view() const {
        return to_sv();
    }
};

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

    infd_base_t(const infd_base_t&) = delete;
    infd_base_t& operator=(const infd_base_t&) = delete;

    infd_base_t(infd_base_t&& ifd) noexcept:
        fd(ifd.fd),
        size(ifd.size),
        pos(ifd.pos),
        wait_timeout(ifd.wait_timeout),
        can_blocked(ifd.can_blocked),
        stat_executed(ifd.stat_executed) {
        std::memcpy(buf + ifd.pos, ifd.buf + ifd.pos, ifd.size * sizeof(T));
        ifd.fd = -1;
    }

    infd_base_t& operator=(infd_base_t&& ifd) noexcept {
        if (this == &ifd)
            return *this;

        if (fd >= 0)
            Impl::impl_close(fd);

        fd = ifd.fd;
        size = ifd.size;
        pos = ifd.pos;
        wait_timeout = ifd.wait_timeout;
        can_blocked = ifd.can_blocked;
        stat_executed = ifd.stat_executed;

        std::memcpy(buf + ifd.pos, ifd.buf + ifd.pos, ifd.size * sizeof(T));
        ifd.fd = -1;

        return *this;
    }

    ~infd_base_t() {
        if (fd >= 0)
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
        if (can_be_blocked()) {
            if (Impl::impl_waitdev(fd, wait_timeout.count()))
                return read(destination, count);
            else
                return {0, io_read_rc::timeout};
        }
        else
            return read(destination, count);
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
    friend class infd_scroll_range_base<T, infd_base_t>;

    class scroll_iterator {
    public:
        scroll_iterator& operator++() {
            if (!block_on_fifo && prev_rc == io_read_rc::partial) {
                ifd = nullptr;
                return *this;
            }

            prev_rc = ifd->take_next();
            if (prev_rc == io_read_rc::error || (!block_on_fifo && ifd->size == 0))
                ifd = nullptr;

            return *this;
        }

        bool operator==(const scroll_iterator& i) const {
            return ifd == i.ifd;
        }

        bool operator!=(const scroll_iterator& i) const {
            return !(*this == i);
        }

        infd_scroll_range<T, infd_base_t> operator*() const {
            return {ifd};
        }

    private:
        friend infd_base_t;
        scroll_iterator(infd_base_t* ifd_ = nullptr, bool iblock_on_fifo = true):
            ifd(ifd_), block_on_fifo(iblock_on_fifo) {
            if (!ifd)
                return;

            if (ifd->fd < 0) {
                ifd = nullptr;
                return;
            }

            if (ifd->can_be_blocked()) {
                if (!block_on_fifo && !Impl::impl_waitdev(ifd->fd, ifd->wait_timeout.count())) {
                    ifd = nullptr;
                    return;
                }
            }
            ++(*this);
        }

    private:
        infd_base_t* ifd;
        io_read_rc   prev_rc = io_read_rc::ok;
        bool         block_on_fifo;
    };

    friend scroll_iterator;

    struct scroll {
        scroll_iterator begin() const {
            return {_ifd, block_on_fifo};
        }

        scroll_iterator end() const {
            return {};
        }

        infd_base_t* _ifd;
        bool block_on_fifo;
    };

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

public:
    scroll scroll(bool block_on_fifo = true) & {
        return {this, block_on_fifo};
    }

private:
    fd_type                   fd   = Impl::default_infd();
    size_t                    size = 0;
    size_t                    pos  = 0;
    T                         buf[S];
    std::chrono::microseconds wait_timeout{0};
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
