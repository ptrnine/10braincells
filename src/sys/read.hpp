#pragma once

#include <core/array.hpp>
#include <core/concepts/trivial_span_like.hpp>
#include <sys/syscall.hpp>

namespace sys {
template <typename Trap>
struct read_provider {
    auto read(fd_t fd, void* output, size_t size) {
        return syscall<size_t, Trap>(SYS_read, fd, output, size);
    }

    /* Return ENAVAIL if read() returns less than sizeof(T) */
    template <typename T> requires core::trivial<T>
    auto read(fd_t fd) {
        syscall_result<T> result{core::type<T>};

        auto res = read(fd, &result.unsafe_get(), sizeof(T));
        if (!res)
            result.set(res.error());
        else if (*res != sizeof(T))
            result.set(errc{ENAVAIL});

        return result;
    }

    /* Return ENAVAIL if size returned by read() divided by sizeof(T) with remainder */
    auto read(fd_t fd, core::trivial_span_like auto& output) {
        constexpr auto type_size = sizeof(*output.data());
        auto res = read(fd, output.data(), output.size() * type_size);
        if (res) {
            auto remains = *res % type_size;
            if (remains)
                res.set(errc{ENAVAIL});
            else
                *res /= type_size;
        }
        return res;
    }

    /* Return ENAVAIL if size returned by read() divided by sizeof(T) with remainder */
    template <typename T, size_t S> requires core::trivial<T>
    auto read(fd_t fd, T (&output)[S]) {
        constexpr auto type_size = sizeof(T);
        auto res = read(fd, output, S * type_size);
        if (res) {
            auto remains = *res % type_size;
            if (remains)
                res.set(errc{ENAVAIL});
            else
                *res /= type_size;
        }
        return res;
    }
};

template <typename T>
auto read(fd_t fd, T* output, size_t size) {
    return read_provider<trap_async_cancel>{}.read(fd, output, size);
}

/* Return ENAVAIL if read() returns less than sizeof(T) */
template <typename T> requires core::trivial<T>
auto read(fd_t fd) {
    return read_provider<trap_async_cancel>{}.read<T>(fd);
}

/* Return ENAVAIL if size returned by read() divided by sizeof(T) with remainder */
template <typename T, size_t S> requires core::trivial<T>
auto read(fd_t fd, T (&output)[S]) {
    return read_provider<trap_async_cancel>{}.read(fd, output);
}

auto read(fd_t fd, core::trivial_span_like auto& output) {
    return read_provider<trap_async_cancel>{}.read(fd, output);
}

template <typename T>
auto read_no_cp(fd_t fd, T* output, size_t size) {
    return read_provider<void>{}.read(fd, output, size);
}

/* Return ENAVAIL if read() returns less than sizeof(T) */
template <typename T> requires core::trivial<T>
auto read_no_cp(fd_t fd) {
    return read_provider<void>{}.read<T>(fd);
}

/* Return ENAVAIL if size returned by read() divided by sizeof(T) with remainder */
template <typename T, size_t S> requires core::trivial<T>
auto read_no_cp(fd_t fd, T (&output)[S]) {
    return read_provider<void>{}.read(fd, output);
}

auto read_no_cp(fd_t fd, core::trivial_span_like auto& output) {
    return read_provider<void>{}.read(fd, output);
}
} // namespace sys
