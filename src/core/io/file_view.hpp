#pragma once

#include <cstring>

#include <core/io/mmap.hpp>

namespace core {
template <typename T>
class file_view {
public:
    file_view() = default;

    file_view(const char* filename) {
        mfr = decltype(mfr)(filename);
        int fd;
        try {
            mfr = mmap_file_range<T, false, false>(filename);
            return;
        }
        catch (const stat_fd_failed& e) {
            fd = e.unclosed_fd;
        }
        catch (const mmap_fd_failed& e) {
            fd = e.unclosed_fd;
        }

        really_mmaped = false;
        read_buffered(fd);
    }

    file_view(file_view&& fv) noexcept = default;

    file_view& operator=(file_view&& fv) noexcept {
        if (&fv == this)
            return *this;

        if (!really_mmaped) {
            delete[] mfr.start;
            mfr.start = nullptr;
            mfr.pend  = nullptr;
        }
        mfr = std::move(fv.mfr);
        really_mmaped = fv.really_mmaped;

        return *this;
    }

    ~file_view() {
        if (!really_mmaped) {
            delete [] mfr.start;
            mfr.start = nullptr;
            mfr.pend = nullptr;
        }
    }

    const T* begin() const {
        return mfr.begin();
    }

    const T* end() const {
        return mfr.end();
    }

    bool is_mmaped() const {
        return really_mmaped;
    }

    auto& mmap_range() {
        return mfr;
    }

    auto& mmap_range() const {
        return mfr;
    }

    const T* data() const {
        return begin();
    }

    [[nodiscard]]
    size_t size() const {
        return static_cast<size_t>(end() - begin());
    }

private:
    void read_buffered(int fd) {
        auto scope_exit = finalizer([=] { ::close(fd); });

        static constexpr size_t buff_size = 8192 / sizeof(T);
        T                       buff[buff_size];
        ssize_t                 avail = 0;

        size_t allocated = 0;
        size_t size      = 0;
        auto&  data      = mfr.start;

        while ((avail = ::read(fd, buff, buff_size * sizeof(T))) > 0) {
            auto required_allocated = size + size_t(avail);
            if (allocated < required_allocated) {
                auto new_allocated = allocated * 2;
                if (new_allocated < required_allocated)
                    new_allocated = required_allocated;

                auto new_data = new T[new_allocated]; // NOLINT
                ::memcpy(new_data, data, size * sizeof(T));
                delete[] data; // NOLINT
                data      = new_data;
                allocated = new_allocated;
            }

            ::memcpy(data + size, buff, size_t(avail) * sizeof(T));
            size += size_t(avail);
        }

        if (avail < 0) {
            delete[] data; // NOLINT
            throw sys_read_fail(errc::from_errno());
        }

        mfr.pend = mfr.start + size;
    }

private:
    mmap_file_range<T, false, false> mfr;
    bool                             really_mmaped = true;
};
} // namespace core
