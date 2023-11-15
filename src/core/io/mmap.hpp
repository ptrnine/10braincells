#pragma once

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <core/errc.hpp>
#include <core/finalizers.hpp>
#include <core/io/exceptions.hpp>

namespace core {
template <typename T, bool Mutable = false, bool autoclose_fd_on_fail = true>
class mmap_file_range {
public:
    template <typename>
    friend class file_view;

    using value_type = std::conditional_t<Mutable, T, const T>;

    mmap_file_range() = default;

    mmap_file_range(const char* filename) {
        int fd = open(filename, Mutable ? O_RDWR : O_RDONLY);
        if (fd < 0)
            throw cannot_open_file(filename, errc::from_errno());

        auto guard = exception_guard([&]{
            if constexpr (autoclose_fd_on_fail)
                ::close(fd);
        });

        struct stat st;
        if (fstat(fd, &st) < 0)
            throw stat_fd_failed(filename, errc::from_errno(), autoclose_fd_on_fail ? -1 : fd);

        auto size = static_cast<size_t>(st.st_size) / sizeof(T);
        if (size != 0) {
            if ((start = static_cast<T*>(mmap(nullptr,
                                              size_t(st.st_size),
                                              PROT_READ | (Mutable ? PROT_WRITE : 0),
                                              Mutable ? MAP_SHARED : MAP_PRIVATE,
                                              fd,
                                              0))) == MAP_FAILED) // NOLINT
                throw mmap_fd_failed(filename, errc::from_errno(), autoclose_fd_on_fail ? -1 : fd);

            pend = start + size;
        }

        /* Explicitly close on successfull exit */
        ::close(fd);
    }

    ~mmap_file_range() {
        if (start)
            ::munmap(start, static_cast<size_t>(pend - start));
    }

    mmap_file_range(mmap_file_range&& mfr) noexcept: start(mfr.start), pend(mfr.pend) {
        mfr.start = nullptr;
    }

    mmap_file_range& operator=(mmap_file_range&& mfr) noexcept {
        if (&mfr == this)
            return *this;

        if (start)
            ::munmap(start, static_cast<size_t>(pend - start));

        start     = mfr.start;
        pend      = mfr.pend;
        mfr.start = nullptr;
        return *this;
    }

    const value_type* begin() const {
        return start;
    }

    const value_type* end() const {
        return pend;
    }

    value_type* begin() {
        return start;
    }

    value_type* end() {
        return pend;
    }

    const value_type* data() const {
        return begin();
    }

    value_type* data() {
        return begin();
    }

    [[nodiscard]]
    size_t size() const {
        return static_cast<size_t>(pend - start);
    }

private:
    T* start = nullptr;
    T* pend  = nullptr;
};
} // namespace core
