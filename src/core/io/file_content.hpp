#pragma once

#include <cstring>

#include <core/io/mmap_file_content.hpp>

namespace core {
template <typename T>
class file_content {
public:
    file_content() = default;

    file_content(const char* filename) {
        mfc = decltype(mfc)(filename);
        int fd;
        try {
            mfc = mmap_file_content<T, false, false>(filename);
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

    file_content(const std::string& filename): file_content(filename.data()) {}

    file_content(file_content&& fv) noexcept = default;

    file_content& operator=(file_content&& fv) noexcept {
        if (&fv == this)
            return *this;

        if (!really_mmaped) {
            delete[] mfc.start;
            mfc.start = nullptr;
            mfc.pend  = nullptr;
        }
        mfc = std::move(fv.mfc);
        really_mmaped = fv.really_mmaped;

        return *this;
    }

    ~file_content() {
        if (!really_mmaped) {
            delete [] mfc.start;
            mfc.start = nullptr;
            mfc.pend = nullptr;
        }
    }

    const T* begin() const {
        return mfc.begin();
    }

    const T* end() const {
        return mfc.end();
    }

    [[nodiscard]]
    bool is_mmaped() const {
        return really_mmaped;
    }

    auto& mmap_range() {
        return mfc;
    }

    auto& mmap_range() const {
        return mfc;
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
        auto&  data      = mfc.start;

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

        mfc.pend = mfc.start + size;
    }

private:
    mmap_file_content<T, false, false> mfc;
    bool                               really_mmaped = true;
};
} // namespace core
