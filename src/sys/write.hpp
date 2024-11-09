#pragma once

#include <core/concepts/trivial_span_like.hpp>
#include <sys/syscall.hpp>

namespace sys
{
template <typename Trap>
struct write_provider {
    auto write(fd_t fd, const void* data, size_t size) {
        return syscall<size_t, Trap>(SYS_write, fd, data, size);
    }

    /* Return ENAVAIL if write returns size less then sizeof(data) */
    auto write(fd_t fd, const core::trivial auto& data) {
        auto res = write(fd, &data, sizeof(data));
        if (res && *res < sizeof(data))
            res.set(errc{ENAVAIL});
        return res;
    }

    /* Return ENAVAIL if write returns size less then sizeof(data) */
    auto write(fd_t fd, const core::trivial_span_like auto& data) {
        auto res = write(fd, data.data(), data.size() * sizeof(*data.data()));
        if (res) {
            if (*res < data.size() * sizeof(*data.data()))
                res.set(errc{ENAVAIL});
            else
                *res /= sizeof(*data.data());
        }
        return res;
    }
};

template <typename T>
auto write(fd_t fd, const T* data, size_t size) {
    return write_provider<trap_async_cancel>{}.write(fd, data, size);
}

auto write(fd_t fd, const core::trivial auto& data) {
    return write_provider<trap_async_cancel>{}.write(fd, data);
}

auto write(fd_t fd, const core::trivial_span_like auto& data) {
    return write_provider<trap_async_cancel>{}.write(fd, data);
}

template <typename T>
auto write_no_cp(fd_t fd, const T* data, size_t size) {
    return write_provider<void>{}.write(fd, data, size);
}

auto write_no_cp(fd_t fd, const core::trivial auto& data) {
    return write_provider<void>{}.write(fd, data);
}

auto write_no_cp(fd_t fd, const core::trivial_span_like auto& data) {
    return write_provider<void>{}.write(fd, data);
}
} // namespace sys
