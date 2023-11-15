#pragma once

#include <cstring>

#include <core/io/impl.hpp>

namespace core {
template <typename DerivedT, typename Impl, typename T = char, size_t S = 8192, auto... Settings>
class outfd_base_t : public Impl { // NOLINT
public:
    template <typename DerivedT2, typename Impl2, typename TT, size_t SS, auto... SSettings>
    friend class outfd_base_t;

    using fd_type = decltype(Impl::impl_open("", fd_flag{}, file_permissions{}));

    static constexpr bool exception_on_syswrite_fail =
        details::check_enable<fd_exception_on_syswrite_fail>(Settings...);
    static constexpr bool exception_on_seek_fail = details::check_enable<fd_exception_on_seek_fail>(Settings...);

    constexpr static DerivedT raw_create(fd_type fd) {
        DerivedT of;
        of.fd = fd;
        return of;
    }

    outfd_base_t() = default;

    outfd_base_t(const char* filename, file_permissions create_permissions):
        fd(Impl::impl_open(filename, fd_flag::write_only | fd_flag::create | fd_flag::trunc, create_permissions)) {}

    outfd_base_t(const char*      filename,
                 fd_combined_flag flags              = fd_flag::write_only | fd_flag::create | fd_flag::trunc,
                 file_permissions create_permissions = file_permissions::user_rw | file_permissions::group_read |
                                                       file_permissions::other_read):
        fd(Impl::impl_open(filename, flags, create_permissions)) {}

    outfd_base_t(const std::string& filename, file_permissions create_permissions):
        outfd_base_t(filename.data(), create_permissions) {}

    outfd_base_t(const std::string& filename,
                 fd_combined_flag   flags              = fd_flag::write_only | fd_flag::create | fd_flag::trunc,
                 file_permissions   create_permissions = file_permissions::user_rw | file_permissions::group_read |
                                                       file_permissions::other_read):
        outfd_base_t(filename.data(), flags, create_permissions) {}

    outfd_base_t(const outfd_base_t&) = delete;
    outfd_base_t& operator=(const outfd_base_t&) = delete;

    constexpr static bool is_greater_multiple_of_smaller(auto one, auto two) {
        return (one < two || one % two == 0) && (two < one || two % one == 0);
    }

    template <typename TT>
    constexpr static bool size_cast(size_t sz) {
        return sizeof(TT) > sizeof(T) ? sz * (sizeof(TT) / sizeof(T)) : sz / (sizeof(T) / sizeof(TT));
    }

    template <typename DerivedT2, typename Impl2, typename TT, size_t SS, auto... SSettings>
    outfd_base_t(outfd_base_t<DerivedT2, Impl2, TT, SS, SSettings...>&& of) noexcept(
        !exception_on_syswrite_fail ||
        (is_greater_multiple_of_smaller(sizeof(T), sizeof(TT)) && sizeof(SS) <= sizeof(S))):
        fd(of.fd) {

        if constexpr (!is_greater_multiple_of_smaller(sizeof(T), sizeof(TT)))
            of.flush();

        auto can_take_buff = of.size * sizeof(TT) <= S * sizeof(T);
        if (can_take_buff) {
            ::memcpy(buf, of.buf, of.size * sizeof(TT));
            size    = size_cast<TT>(of.size);
            pos     = size_cast<TT>(of.pos);
        }
        else {
            of.flush();
            size = 0;
            pos  = 0;
        }
        of.fd = STDOUT_FILENO;
    }

    template <typename DerivedT2, typename Impl2, typename TT, size_t SS, auto... SSettings>
    outfd_base_t&
    operator=(outfd_base_t<DerivedT2, Impl2, TT, SS, SSettings...>&& of) noexcept(!exception_on_syswrite_fail) {
        if constexpr (std::is_same_v<DerivedT, DerivedT2>) {
            if (&of == this)
                return *this;
        }

        if constexpr (!is_greater_multiple_of_smaller(sizeof(T), sizeof(TT)))
            of.flush();

        if (size != 0)
            Impl::impl_write(fd, buf, size * sizeof(T));

        Impl::impl_close(fd);

        fd = of.fd;

        auto can_take_buff = of.size * sizeof(TT) <= S * sizeof(T);
        if (can_take_buff) {
            ::memcpy(buf, of.buf, of.size * sizeof(TT));
            size    = size_cast<TT>(of.size);
            pos     = size_cast<TT>(of.pos);
            of.size = 0;
            of.pos  = 0;
        }
        else {
            of.flush();
            size = 0;
            pos  = 0;
        }
        of.fd = 0;
        return *this;
    }

    outfd_base_t(outfd_base_t&& ofd) noexcept:
        fd(ofd.fd), size(ofd.size), pos(ofd.pos), is_fifo_fd(ofd.is_fifo_fd), stat_executed(ofd.stat_executed) {
        std::memcpy(buf, ofd.buf, ofd.size * sizeof(T));
        ofd.fd = -1;
    }

    outfd_base_t& operator=(outfd_base_t&& ofd) noexcept {
        if (this == &ofd)
            return *this;

        destroy();

        fd = ofd.fd;
        size = ofd.size;
        pos = ofd.pos;
        is_fifo_fd = ofd.is_fifo_fd;
        stat_executed = ofd.stat_executed;

        std::memcpy(buf, ofd.buf, ofd.size * sizeof(T));
        ofd.fd = -1;

        return *this;
    }

    ~outfd_base_t() {
        destroy();
    }

    DerivedT& write(const T* data, size_t count) noexcept(!exception_on_syswrite_fail) {
        if (count >= S) {
            /* Data do not actualy wraps into buffer */
            flush();
            Impl::impl_write(fd, data, count * sizeof(T));
        }
        else {
            auto free_space = S - pos;
            if (free_space < count)
                flush();

            ::memcpy(buf + pos, data, count * sizeof(T));
            pos += count;

            if (size < pos)
                size = pos;
        }
        return static_cast<DerivedT&>(*this);
    }

    /* May violate strict aliasing rule */
    DerivedT& flatcopy(auto&&... some) {
        return (write(reinterpret_cast<const T*>(&some), sizeof(some)), ...); // NOLINT
    }

    template <size_t N>
    DerivedT& write_any(const char(&c_string)[N]) noexcept(!exception_on_syswrite_fail) {
        return write(c_string, N - 1);
    }

    DerivedT& write_any(std::convertible_to<T> auto&& single_char) noexcept(!exception_on_syswrite_fail) {
        return put(single_char);
    }

    DerivedT& write_any(outfd_concepts::WritableBase<T> auto&& something_flat) noexcept(!exception_on_syswrite_fail) {
        using std::begin, std::end;
        return write(begin(something_flat),
                     static_cast<size_t>(end(something_flat) - begin(something_flat)));
    }

    DerivedT& write_any(outfd_concepts::WritableRAI<T> auto&& rai_container) noexcept(!exception_on_syswrite_fail) {
        using std::begin, std::end;
        return write(begin(rai_container).base(),
                     static_cast<size_t>(end(rai_container) - begin(rai_container)));
    }

    DerivedT& writesome(outfd_concepts::Writable<T> auto&&... some) noexcept(!exception_on_syswrite_fail) {
        (write_any(some), ...);
        return static_cast<DerivedT&>(*this);
    }

    bool flush() noexcept(!exception_on_syswrite_fail) {
        if (size) {
            Impl::impl_write(fd, buf, sizeof(T) * pos);

            if (pos < size) {
                ::memmove(buf, buf + pos, (size - pos) * sizeof(T));
                size = size - pos;
            }
            else
                size = 0;

            pos = 0;
            return true;
        }
        return false;
    }

    DerivedT& put(T data) noexcept(!exception_on_syswrite_fail) {
        if (size >= S)
            flush();

        buf[pos++] = data;

        if (pos > size)
            size = pos;

        return static_cast<DerivedT&>(*this);
    }

    DerivedT& seek(ssize_t value) noexcept(!exception_on_seek_fail && !exception_on_syswrite_fail) {
        if (value == 0)
            return static_cast<DerivedT&>(*this);

        if ((value < 0 && -value <= pos) || (value > 0 && value <= size - pos)) {
            pos += value;
            return static_cast<DerivedT&>(*this);
        }

        flush();
        Impl::impl_seek(fd, value);
        return static_cast<DerivedT&>(*this);
    }

    bool is_fifo() const {
        if (!stat_executed) {
            is_fifo_fd   = Impl::impl_is_blocking(fd);
            stat_executed = true;
        }
        return is_fifo_fd;
    }

    const fd_type& descriptor() const {
        return fd;
    }

private:
    void destroy() {
        if (fd < 0)
            return;

        if (size != 0)
            Impl::impl_write(fd, buf, size * sizeof(T));
        Impl::impl_close(fd);
    }

private:
    fd_type fd   = Impl::default_outfd();
    size_t  size = 0;
    size_t  pos  = 0;
    T       buf[S];

    mutable bool is_fifo_fd    = false;
    mutable bool stat_executed = false;
};

#define DECLARE_OUTFD(NAME, IMPL)                                                                                      \
    template <typename T = char, size_t S = 8192, auto... Settings>                                                    \
    struct NAME : public outfd_base_t<NAME<T, S, Settings...>,                                                         \
                                      IMPL<NAME<T, S, Settings...>, Settings...>,                                      \
                                      T,                                                                               \
                                      S,                                                                               \
                                      Settings...> {                                                                   \
        using outfd_base_t<NAME<T, S, Settings...>, IMPL<NAME<T, S, Settings...>, Settings...>, T, S, Settings...>::   \
            outfd_base_t;                                                                                              \
    }

DECLARE_OUTFD(outfd, outfd_linux_impl_t);
DECLARE_OUTFD(outfd_devnull, outfd_devnull_impl);
} // namespace core
