#pragma once

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <core/errc.hpp>
#include <core/exception_guard.hpp>
#include <core/io/exceptions.hpp>

namespace core {
template <typename T, bool Mutable = false, bool autoclose_fd_on_fail = true>
class mmap_file_content {
public:
    template <typename>
    friend class file_view;

    using value_type = std::conditional_t<Mutable, T, const T>;

    mmap_file_content() = default;

    mmap_file_content(const char* filename, bool create_new = false, size_t preferred_size = 0) {
        int fd = Mutable && create_new ? ::open(filename, O_RDWR | O_CREAT, 0644)
                                       : ::open(filename, Mutable ? O_RDWR : O_RDONLY);
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
        if (Mutable && size < preferred_size) {
            if (::ftruncate(fd, off_t(preferred_size * sizeof(T))) < 0)
                throw ftruncate_fd_failed(filename, errc::from_errno());
            size = preferred_size;
        }

        if (size != 0) {
            if ((start = static_cast<T*>(mmap(nullptr,
                                              size * sizeof(T),
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

    mmap_file_content(const std::string& filename, bool create_new = false, size_t preferred_size = 0):
        mmap_file_content(filename.data(), create_new, preferred_size) {}

    ~mmap_file_content() {
        if (start)
            ::munmap(start, static_cast<size_t>(pend - start));
    }

    mmap_file_content(mmap_file_content&& mfr) noexcept: start(mfr.start), pend(mfr.pend) {
        mfr.start = nullptr;
    }

    mmap_file_content& operator=(mmap_file_content&& mfr) noexcept {
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
