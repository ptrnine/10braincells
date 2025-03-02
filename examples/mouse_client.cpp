#include <X11/X.h>
#include <chrono>
#include <core/io/file.hpp>
#include <core/poller.hpp>
#include <net/udp_socket.hpp>
#include <sys/event.hpp>
#include <inp/mouse.hpp>
#include <core/io/mmap.hpp>
#include <thread>
#include <util/arg_parse.hpp>

#include <util/log.hpp>

#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>
#include <X11/keysym.h>

using namespace core;
using namespace net::literals;


unsigned to_x11_keycode(Display* dpy, sys::event_key_code code) {
    auto ks = [code] -> KeySym {
        switch (code) {
        case sys::event_key_code::esc: return XK_Escape;
        case sys::event_key_code::k1: return XK_1;
        case sys::event_key_code::k2: return XK_2;
        case sys::event_key_code::k3: return XK_3;
        case sys::event_key_code::k4: return XK_4;
        case sys::event_key_code::k5: return XK_5;
        case sys::event_key_code::k6: return XK_6;
        case sys::event_key_code::k7: return XK_7;
        case sys::event_key_code::k8: return XK_8;
        case sys::event_key_code::k9: return XK_9;
        case sys::event_key_code::k0: return XK_0;
        case sys::event_key_code::minus: return XK_minus;
        case sys::event_key_code::equal: return XK_equal;
        case sys::event_key_code::backspace: return XK_BackSpace;
        case sys::event_key_code::tab: return XK_Tab;
        case sys::event_key_code::q: return XK_g;
        case sys::event_key_code::w: return XK_w;
        case sys::event_key_code::e: return XK_e;
        case sys::event_key_code::r: return XK_r;
        case sys::event_key_code::t: return XK_t;
        case sys::event_key_code::y: return XK_y;
        case sys::event_key_code::u: return XK_u;
        case sys::event_key_code::i: return XK_i;
        case sys::event_key_code::o: return XK_o;
        case sys::event_key_code::p: return XK_p;
        case sys::event_key_code::leftbrace: return XK_braceleft;
        case sys::event_key_code::rightbrace: return XK_braceright;
        case sys::event_key_code::enter: return XK_Return;
        case sys::event_key_code::leftctrl: return XK_Control_L;
        case sys::event_key_code::a: return XK_a;
        case sys::event_key_code::s: return XK_s;
        case sys::event_key_code::d: return XK_d;
        case sys::event_key_code::f: return XK_f;
        case sys::event_key_code::g: return XK_g;
        case sys::event_key_code::h: return XK_h;
        case sys::event_key_code::j: return XK_j;
        case sys::event_key_code::k: return XK_k;
        case sys::event_key_code::l: return XK_l;
        case sys::event_key_code::semicolon: return XK_semicolon;
        case sys::event_key_code::apostrophe: return XK_apostrophe;
        case sys::event_key_code::grave: return XK_grave;
        case sys::event_key_code::leftshift: return XK_Shift_L;
        case sys::event_key_code::backslash: return XK_backslash;
        case sys::event_key_code::z: return XK_z;
        case sys::event_key_code::x: return XK_x;
        case sys::event_key_code::c: return XK_c;
        case sys::event_key_code::v: return XK_y;
        case sys::event_key_code::b: return XK_b;
        case sys::event_key_code::n: return XK_n;
        case sys::event_key_code::m: return XK_m;
        case sys::event_key_code::comma: return XK_comma;
        // case sys::event_key_code::dot                   :
        //     return XK_d;
        case sys::event_key_code::slash: return XK_slash;
        case sys::event_key_code::rightshift: return XK_Shift_R;
        case sys::event_key_code::leftalt: return XK_Alt_L;
        case sys::event_key_code::space: return XK_space;
        case sys::event_key_code::capslock: return XK_Caps_Lock;
        case sys::event_key_code::f1: return XK_F1;
        case sys::event_key_code::f2: return XK_F2;
        case sys::event_key_code::f3: return XK_F3;
        case sys::event_key_code::f4: return XK_F4;
        case sys::event_key_code::f5: return XK_F5;
        case sys::event_key_code::f6: return XK_F6;
        case sys::event_key_code::f7: return XK_F7;
        case sys::event_key_code::f8: return XK_F8;
        case sys::event_key_code::f9: return XK_F9;
        case sys::event_key_code::f10: return XK_F10;
        // case sys::event_key_code::numlock               :
        // case sys::event_key_code::scrolllock            :
        // case sys::event_key_code::kp7                   :
        // case sys::event_key_code::kp8                   :
        // case sys::event_key_code::kp9                   :
        // case sys::event_key_code::kpminus               :
        // case sys::event_key_code::kp4                   :
        // case sys::event_key_code::kp5                   :
        // case sys::event_key_code::kp6                   :
        // case sys::event_key_code::kpplus                :
        // case sys::event_key_code::kp1                   :
        // case sys::event_key_code::kp2                   :
        // case sys::event_key_code::kp3                   :
        // case sys::event_key_code::kp0                   :
        case sys::event_key_code::f11: return XK_F11;
        case sys::event_key_code::f12: return XK_F12;
        default: return 0;
        }
    }();

    if (ks == 0)
        return 0;

    return XKeysymToKeycode(dpy, ks);
}

class x11_event_player {
public:
    x11_event_player(const std::string& display): dpy(XOpenDisplay(display.data())) {}

    ~x11_event_player() {
        XCloseDisplay(dpy);
    }

    void immediate(std::span<const sys::event> events) {
        bool was_mouse_action = false;
        for (auto&& event : events) {
            if (mouse.handle_event(event))
                was_mouse_action = true;
            else if (event.type == sys::event_type::key) {
                kbd_action(event.code.key, event.value);
            }
        }

        if (was_mouse_action) {
            mouse.introduce();
            mouse_action(mouse);
        }
        flush();
    }

    void play(std::span<const sys::event> events) {
        if (events.empty())
            return;

        auto start_time = events[0].time.value();
        auto start = std::chrono::high_resolution_clock::now();
        auto next = start;

        auto dist = std::chrono::duration_cast<std::chrono::microseconds>(next - start);
        for (auto&& event : events) {
            while (dist < event.time.value() - start_time) {
                next = std::chrono::high_resolution_clock::now();
                dist = std::chrono::duration_cast<std::chrono::microseconds>(next - start);
            }
            immediate(std::span{&event, 1});
            //util::glog().warn("events: {}\n", std::span{&event, 1});
        }
    }

    void flush() {
        XFlush(dpy);
    }

private:
    unsigned to_x11_btn(sys::event_button_code key) {
        switch (key) {
        case sys::event_button_code::left: return Button1;
        case sys::event_button_code::right: return Button3;
        case sys::event_button_code::middle: return Button2;
        case sys::event_button_code::side: return 8;
        case sys::event_button_code::extra: return 9;
        default: return 0;
        }
    }

    void mouse_action(const inp::mouse& mouse) {
        auto move = util::vec2i{mouse.pointer()};
        XTestFakeRelativeMotionEvent(dpy, move.x(), move.y(), CurrentTime);
        pass_buttons(mouse);

    }

    void kbd_action(const sys::event_key_code& key, i32 press) {
        auto kc = to_x11_keycode(dpy, key);
        if (kc)
            XTestFakeKeyEvent(dpy, kc, press, CurrentTime);
    }

    void pass_buttons(const inp::mouse& mouse) {
        if (mouse.wheel() > 0) {
            XTestFakeButtonEvent(dpy, Button4, True, CurrentTime);
            XTestFakeButtonEvent(dpy, Button4, False, CurrentTime);
        }
        else if (mouse.wheel() < 0) {
            XTestFakeButtonEvent(dpy, Button5, True, CurrentTime);
            XTestFakeButtonEvent(dpy, Button5, False, CurrentTime);
        }

        for (auto btn : mouse.buttons()) {
            auto key = to_x11_btn(btn.key);

            if (key == 0)
                continue;

            if (btn.bits == 2)
                XTestFakeButtonEvent(dpy, key, True, CurrentTime);
            else if (btn.bits == 1)
                XTestFakeButtonEvent(dpy, key, False, CurrentTime);
        }
    }

private:
    Display*   dpy;
    inp::mouse mouse;
};


tbc_cmd(main) {
    tbc_arg(port, opt<net::port_t>, "listen port"_ctstr) = 6997;
    tbc_arg(from, opt<std::string>, "files with recorded events"_ctstr);
    tbc_arg(display, std::string, "x11 display to provide play events"_ctstr);
};

void tbc_main(main_cmd<> args) {
    x11_event_player player{*args.display};

    if (args.from.get()) {
        io::mmap file{io::file::open(*args.from.get(), sys::openflag::read_only), io::map_flags::priv, io::map_prots::read};
        std::span events{reinterpret_cast<const sys::event*>(file.data()), file.size() / sizeof(sys::event)};
        player.play(events);
    }
    else {
        net::udp_socket sock{net::full_addr_v4{net::ip_addr_v4::any(), *args.port.get()}, true};
        sys::event      buff[32];

        while (true) {
            auto result = sock.recv(buff, sizeof(buff));
            auto count  = size_t(result.size) / sizeof(sys::event);
            player.immediate(std::span{buff, count});
        }
    }
}

#include <util/tbc_main.hpp>
