#pragma once

#include <GLFW/glfw3.h>

#include <inp/mouse.hpp>

#define fwd(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace inp
{
class glfw_mouse {
public:
    void introduce() {
        _m.introduce();
    }

    void handle_pos(double x, double y) {
        if (!_init) {
            prev_x = sys::i32(x);
            prev_y = sys::i32(y);
            _init = true;
            return;
        }

        sys::i32 dx = sys::i32(x) - prev_x;
        sys::i32 dy = sys::i32(y) - prev_y;
        prev_x = sys::i32(x);
        prev_y = sys::i32(y);

        sys::event evt;
        evt.type     = sys::event_type::relative;
        evt.code.rel = sys::event_relative_code::x;
        evt.value    = dx;
        _m.handle_event(evt);
        evt.code.rel = sys::event_relative_code::y;
        evt.value    = dy;
        _m.handle_event(evt);
    }

    void handle_button(int button, int action) {
        auto btn = sys::event_button_code(button + int(sys::event_button_code::left));
        sys::event evt;
        evt.type     = sys::event_type::key;
        evt.code.btn = btn;
        evt.value    = action;
        _m.handle_event(evt);
    }

    auto&& operator*(this auto&& it) {
        return fwd(it)._m;
    }

    auto operator->(this auto&& it) {
        return &fwd(it)._m;
    }

    const mouse& state() const {
        return _m;
    }

private:
    sys::i32 prev_x, prev_y;
    bool     _init = false;
    mouse    _m;
};
} // namespace inp

#undef fwd
