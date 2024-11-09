#pragma once

#include <atomic>
#include <core/concepts/trivial_span_like.hpp>
#include <core/errc.hpp>
#include <core/opt.hpp>
#include <core/traits/is_ref.hpp>
#include <sys/lseek.hpp>
#include <sys/read.hpp>
#include <sys/statx.hpp>
#include <sys/syscall.hpp>
#include <sys/write.hpp>
#include <core/io/file.hpp>
#include <core/io/basic_types.hpp>

#define fwd(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace core::io
{
template <typename T>
class rw_impl {
public:
    constexpr rw_impl(auto&& buff, bool from_start): storage(fwd(buff)), pos(from_start ? 0 : storage.size()) {}
    constexpr rw_impl(bool from_start): pos(from_start ? 0 : storage.size()) {}

    constexpr auto&& base_buff(this auto&& it) {
        return fwd(it).storage;
    }

    constexpr bool is_pipe_like() const {
        return false;
    }

    constexpr auto avail() const {
        return storage.size() - pos;
    }

protected:
    constexpr size_t handle_write(const void* data, size_t size) {
        auto p    = storage.size();
        storage.resize(pos + size);
        __builtin_memcpy(storage.data() + p, data, size);
        pos += size;
        return size;
    }

    constexpr size_t handle_read(void* data, size_t size) {
        auto sz = avail();
        if (sz > size)
            sz = size;
        __builtin_memcpy(data, storage.data() + pos, sz);
        pos += sz;
        return sz;
    }

    constexpr size_t handle_read(core::trivial auto& data) {
        if (storage.size() - pos < sizeof(data))
            throw errc_exception(errc::enavail);

        __builtin_memcpy(data.data(), storage.data() + pos, sizeof(data));
        pos += sizeof(data);
        return sizeof(data);
    }

    constexpr off_t handle_seek(off_t offset, seek_whence whence) {
        auto new_pos = off_t(pos);
        switch (whence) {
        case seek_whence::cur: new_pos = off_t(pos) + offset; break;
        case seek_whence::set: new_pos = offset; break;
        case seek_whence::end:
            throw errc_exception(errc::einval);
        }

        if (new_pos < 0)
            new_pos = 0;
        else if (new_pos > storage.size())
            new_pos = storage.size();

        return off_t(pos = size_t(new_pos));
    }

private:
    T      storage;
    size_t pos = 0;
};

template <typename T> requires same_as<T, fd_t> || same_as<T, file>
class rw_impl<T> {
public:
    constexpr rw_impl(bool) {}
    constexpr rw_impl(auto&& init, bool): _fd(fwd(init)) {}

    constexpr auto& fd() const {
        return _fd;
    }

    bool is_pipe_like() const {
        switch (fd_type()) {
        case sys::file_type::socket:
        case sys::file_type::fifo:
        case sys::file_type::chardev: return true;
        default: return false;
        }
    }

protected:
    size_t handle_write(const void* data, size_t size) {
        return sys::write(_fd, data, size).get();
    }

    size_t handle_read(void* data, size_t size) {
        auto res = sys::read(_fd, data, size);
        if (!is_pipe_like() || res)
            return res.get();
        else
            return 0;
    }

    off_t handle_seek(off_t offset, seek_whence whence) {
        return sys::lseek(_fd, offset, whence).get();
    }

    sys::file_type fd_type() const {
        auto file_type = _file_type.load(std::memory_order_relaxed);
        if (!file_type) {
            file_type = sys::statx(_fd, sys::statx_mask::mode).get().mode.type();
            _file_type.store(file_type, std::memory_order_relaxed);
        }
        return *file_type;
    }

private:
    T                                              _fd;
    mutable std::atomic<core::opt<sys::file_type>> _file_type;
};
} // namespace core::io

#undef fwd
