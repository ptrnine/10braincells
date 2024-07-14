#pragma once

#include <core/moveonly_trivial.hpp>
#include <inp/mouse.hpp>
#include <sys/close.hpp>
#include <sys/open.hpp>

#define fwd(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace inp
{
template <size_t EventBuffSize = 8>
class native_mouse {
public:
    constexpr native_mouse(const char* path):
        _fd(sys::open(path, sys::openflag::read_only | sys::openflag::nonblock).get()) {}

    constexpr ~native_mouse() {
        if (_fd.not_default())
            sys::close(_fd);
    }

    constexpr native_mouse(native_mouse&&) noexcept = default;
    constexpr native_mouse& operator=(native_mouse&&) noexcept = default;

    void introduce() {
        _m.introduce();
    }

    bool handle() {
        if (_fd.not_default())
            return _m.rt_state.handle_from_fd(_fd, _buff);
        return false;
    }

    auto&& operator*(this auto&& it) {
        return fwd(it)._m;
    }

    auto operator->(this auto&& it) {
        return &fwd(it)._m;
    }

    sys::fd_t fd() const {
        return _fd;
    }

    const mouse& state() const {
        return _m;
    }

private:
    core::moveonly_trivial<sys::fd_t, sys::invalid_fd> _fd;
    core::array<sys::event, EventBuffSize>             _buff;
    mouse                                              _m;
};
} // namespace inp

#undef fwd
