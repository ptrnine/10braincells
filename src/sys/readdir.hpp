#pragma once

#include <sys/dirent.hpp>
#include <sys/syscall.hpp>

namespace sys {
template <size_t BuffSize = dirent_default_buffer_size>
auto getdents(fd_t fd) {
    dirent_result<u8[BuffSize]> res_buff;

    auto res = syscall<size_t>(SYS_getdents64, int(fd), res_buff.buffer(), res_buff.capacity());
    return res.map([&res_buff](size_t size) {
        res_buff.set_size(size_t(size));
        return core::mov(res_buff);
    });
}

auto getdents(fd_t fd, std::span<u8> buff) {
    dirent_result<u8*> res_buff{buff.data(), buff.size()};

    auto res = syscall<size_t>(SYS_getdents64, int(fd), res_buff.buffer(), res_buff.capacity());
    return res.map([&res_buff](size_t size) {
        res_buff.set_size(size_t(size));
        return core::mov(res_buff);
    });
}

struct readdir_iterator_sentinel {};

class readdir_iterator {
public:
    readdir_iterator(fd_t fd, std::span<u8> buff): _fd(core::mov(fd)), _buff(getdents(_fd, buff).get()) {
        if (_buff.is_end()) {
            end = true;
        }
    }

    readdir_iterator& operator++() {
        _buff.next();
        if (_buff.is_end()) {
            _buff = getdents(_fd, std::span{_buff.buffer(), _buff.capacity()}).get();
            if (_buff.is_end()) {
                end = true;
            }
        }
        return *this;
    }

    dir_entry operator*() const {
        return _buff.get();
    }

    bool is_end() {
        return end;
    }

    bool operator==(readdir_iterator_sentinel) const {
        return end;
    }

    bool operator!=(readdir_iterator_sentinel) const {
        return !end;
    }

private:
    fd_t               _fd;
    dirent_result<u8*> _buff;
    bool               end = false;
};

template <size_t BuffSize = dirent_default_buffer_size, typename Fd = fd_t>
class readdir_result {
public:
    readdir_result(Fd fd): _fd(core::mov(fd)) {}

    auto begin() {
        return readdir_iterator{_fd, buff};
    }

    auto end() {
        return readdir_iterator_sentinel{};
    }

private:
    Fd _fd;
    u8 buff[BuffSize];
};

template <size_t BuffSize = dirent_default_buffer_size, typename Fd = fd_t>
auto readdir(Fd fd) {
    return readdir_result<BuffSize>{core::mov(fd)};
}
} // namespace sys
