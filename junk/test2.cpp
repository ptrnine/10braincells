#include <ranges>
#include <sched.h>
#include <sys/poll.h>
#include <linux/input.h>

#include <grx/gl/gl_window.hpp>

#include <core/ranges/subst.hpp>
#include <core/io/std.hpp>
#include <core/ranges/fold.hpp>
#include <sys/poll.hpp>
#include <util/log.hpp>

using namespace util;
using namespace grx;
using namespace core;

namespace stdviews = std::ranges::views;

#include <thread>

#include <sys/sigprocmask.hpp>
#include <sys/signalfd.hpp>
#include <sys/read.hpp>
#include <sys/open.hpp>
#include <sys/event.hpp>
#include <sys/epoll.hpp>

#include <inp/native_mouse.hpp>
#include <inp/glfw_mouse.hpp>

#include <core/poller.hpp>
#include <core/epoller.hpp>
#include <input_mgr.hpp>
#include <engine.hpp>

int main() {
    auto& engine = tbc::engine();
    engine.event_polling.start();

    inp::generic_mouse mouse_dev{type<inp::native_mouse<>>, "/dev/input/event15"};
    engine.input_mgr.attach_mouse(mouse_dev);
    auto& mouse = mouse_dev.state();


    auto now = std::chrono::system_clock::now();
    while ((std::chrono::system_clock::now() - now) < std::chrono::seconds(2)) {
        mouse_dev.introduce();
        glog().warn("pos: {}", mouse.pointer());
        std::this_thread::sleep_for(sys::milliseconds(5));
    }



    //inp_mgr.detach_mouse(nmouse);



    /*
    poller poller{
        poll_handle{
            sys::open("/dev/input/event9", sys::openflag::read_only | sys::openflag::nonblock).get(),
            sys::poll_event::in,
            [&](const sys::pollfd& pfd) {
                sys::event events[1024];
                for (auto&& evt : events | stdviews::take(read(pfd.fd, events).get()))
                    mouse.handle(evt);
                    //if (evt.type != sys::event_type::synchronize)
                    //    glog().info("{}", evt);
            },
        },
    };
    poller.run();
    */


    /*
    auto epfd = sys::epoll_create().get();
    auto fd = sys::open("/dev/input/event9", sys::openflag::read_only | sys::openflag::nonblock).get();
    sys::epoll_ctl_add(epfd, fd, sys::epoll_event::in | sys::epoll_event::edge_triggered, 0xffffffffffffffff).throw_if_error();

    while (true) {
        sys::epoll_data data;
        sys::epoll_wait(epfd, data).throw_if_error();

        sys::event events[1024];
        while (true) {
            auto res = read(fd, events);
            if (!res.ok() && res.error() == errc::eagain)
                break;
            for (auto&& evt : events | stdviews::take(res.get()))
                mouse.handle(evt);
        }
    }
    */

    //t.join();

    // gl::gl_window wnd{{800, 600}};
    // wnd.set_title("pidor 228");
    // wnd.enable_mouse_pointer(false);

    // while (true) {};

    // int   test = 0;
    // auto  p    = &test;
    // auto& cm   = as_const(m);

    // m[p] = "hello";

    // auto& s = (*cm.begin())[int_c<1>];

    // for (auto&& [k, v] : cm)
    //     glog().warn("{}: {}", k, v);

    // postprocess p((gl::gl_program*)0, [](auto&& v) {});
    // p.update();

    // static_assert(sizeof([a = 2]{}) == 4);
    // auto txtr =
    // texture_rgb::from_image(mmap_file_content<byte>("/home/ptrnine/Загрузки/cBn3TXagEdw.jpg"));
    // txtr = txtr.resized({4096, 4096}, 8);
    // outfd<byte>("/home/ptrnine/Рабочий
    // стол/test2.png").write_any(txtr.rendered_with_mipmaps().to_png());

    // auto txtr =
    // texture_rgb::from_image(mmap_file_content<byte>("/home/ptrnine/Загрузки/cBn3TXagEdw.jpg"));
    // auto tx = txtr.resized({512, 512}, 8);
    // outfd<byte>("/home/ptrnine/Рабочий
    // стол/test2.png").write_any(tx.with_mipmaps().to_png());

    /*
    engine_object init_engine;

    engine().dir_watcher.add_handler("./src/core", [](auto path) {
        glog().info("Event in: {}", path);
        return false;
    });

    engine().dir_watcher.add_handler("./src/core", [](auto path) {
        glog().info("Event2 in: {}", path);
        return true;
    });

    std::this_thread::sleep_for(std::chrono::seconds(10));
    */
}
