#pragma once

#include <core/traits/conditional.hpp>
#include <inp/key_rt_state.hpp>
#include <inp/key_switch_state.hpp>
#include <inp/relative_rt_state.hpp>
#include <sys/read.hpp>
#include <util/vec.hpp>

namespace inp {
template <bool Atomic = true>
struct mouse_state {
    using key_block      = core::conditional<Atomic, std::atomic<sys::u16>, sys::u16>;
    using relative_block = core::conditional<Atomic, std::atomic<sys::uint>, sys::uint>;

    static inline constexpr auto btn_key_low  = sys::event_button_code::left;
    static inline constexpr auto btn_key_high = sys::event_button_code::extra;

    using keys_state     = key_rt_state<btn_key_low, btn_key_high, 0, 1, key_block>;
    using relative_state = relative_rt_state<relative_block,
                                             sys::event_relative_code::x,
                                             sys::event_relative_code::y,
                                             sys::event_relative_code::wheel>;

    static inline constexpr sys::i32 pressed  = 1;
    static inline constexpr sys::i32 released = 0;

    static constexpr bool is_atomic() {
        return relative_state::is_atomic() && keys_state::is_atomic();
    }

    constexpr util::vec2f consume_pointer() {
        auto x = _pointer.take(core::int_c<sys::event_relative_code::x>);
        auto y = _pointer.take(core::int_c<sys::event_relative_code::y>);
        return {float(x), float(y)};
    }

    constexpr util::vec2f pointer() const {
        auto x = _pointer.get(core::int_c<sys::event_relative_code::x>);
        auto y = _pointer.get(core::int_c<sys::event_relative_code::y>);
        return {float(x), float(y)};
    }

    constexpr float consume_wheel() {
        return float(_pointer.take(core::int_c<sys::event_relative_code::wheel>));
    }

    constexpr float wheel() const {
        return float(_pointer.get(core::int_c<sys::event_relative_code::wheel>));
    }

    constexpr auto consume_buttons() {
        return _buttons.take_all();
    }

    constexpr auto& buttons() const {
        return _buttons;
    }

    constexpr bool handle_event(const sys::event& event) {
        return event.dispatch<bool>(
            [&](sys::event_relative_code code, sys::i32 value) { return _pointer.handle(code, value); },
            [&](sys::event_button_code code, sys::i32 value) { return _buttons.handle(code, value); });
    }

    bool handle_from_fd(sys::fd_t fd, std::span<sys::event> buffer) {
        bool event_read = false;
        while (true) {
            auto res = sys::read(fd, buffer);
            if (res.error() == core::errc::eagain)
                break;
            auto len = res.get();
            for (size_t i = 0; i < len; ++i)
                event_read |= handle_event(buffer[i]);
        }
        return event_read;
    }

    template <size_t BuffSize = 8>
    bool handle_from_fd(sys::fd_t fd) {
        sys::event events[BuffSize];
        return handle_from_fd(fd, events);
    }

    constexpr inp::mouse_state<false> take_all() {
        return {_pointer.take_all(), _buttons.take_all()};
    }

    relative_state _pointer;
    keys_state     _buttons;
};

class mouse {
public:
    template <size_t>
    friend class native_mouse;

    using sys::event_button_code::left;
    using sys::event_button_code::right;
    using sys::event_button_code::middle;
    using sys::event_button_code::side;
    using sys::event_button_code::extra;

    constexpr mouse() = default;

    constexpr mouse(mouse&& m) noexcept: state(m.rt_state.take_all()) {}
    constexpr mouse& operator=(mouse&& m) noexcept {
        state        = m.rt_state.take_all();
        switch_state = m.switch_state;
        switch_state.update(state._buttons);
        return *this;
    }

    constexpr void introduce() {
        state = rt_state.take_all();
        switch_state.update(state._buttons);
    }

    constexpr bool handle_event(const sys::event& event) {
        return rt_state.handle_event(event);
    }

    constexpr auto pointer() const {
        return state.pointer();
    }

    constexpr auto wheel() const {
        return state.wheel();
    }

    constexpr auto& buttons() const {
        return state.buttons();
    }

    constexpr bool clicked(sys::event_button_code code) const {
        return state.buttons().test(code, state.pressed);
    }

    constexpr bool pressed(sys::event_button_code code) const {
        return switch_state.test(code);
    }

    constexpr bool released(sys::event_button_code code) const {
        return state.buttons().test(code, state.released);
    }

private:
    mouse_state<true>                                                               rt_state;
    mouse_state<false>                                                              state;
    key_switch_state<mouse_state<>::btn_key_low, mouse_state<>::btn_key_high, 1, 0> switch_state;
};
} // namespace inp
