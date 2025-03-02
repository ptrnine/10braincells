#include <core/io/file.hpp>
#include <core/poller.hpp>
#include <net/full_addr.hpp>
#include <net/udp_socket.hpp>
#include <sys/event.hpp>
#include <sys/open_flags.hpp>
#include <sys/read.hpp>
#include <util/arg_parse.hpp>
#include <core/fixed_vector.hpp>

#include <util/log.hpp>

using namespace core;
using namespace net::literals;

#define fwd(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)

tbc_cmd(main) {
    tbc_arg(src_port, opt<net::port_t>, "source port"_ctstr) = 6996;
    tbc_arg(mouse, std::string, "path to mouse input device"_ctstr);
    tbc_arg(kbd, opt<std::string>, "path to keyboard input device"_ctstr);
    tbc_arg(dst, std::string, "destination address or file"_ctstr);
};

auto handle_keyboard(sys::fd_t fd, auto&& events_handler) {
    return poll_handle{
        fd,
        sys::poll_event::in,
        [fd, &events_handler](auto&&) mutable {
            array<sys::event, 4>                  buff;
            fixed_vector<sys::event, buff.size()> filtered;

            if (auto count = sys::read(fd, buff)) {
                for (auto&& event : std::span{buff}.subspan(0, *count)) {
                    event.dispatch([&](sys::event_key_code, i32 value) {
                        /* Pass only press (1) and release (0) */
                        if (value < 2)
                            filtered.emplace_back(event);
                    });
                }
                if (!filtered.empty())
                    events_handler(filtered);
            }
        },
    };
}

auto handle_mouse(sys::fd_t fd, auto&& events_handler) {
    using keycodes = var<sys::event_button_code, sys::event_key_code, sys::event_relative_code, sys::event_absolute_code>;

    return poll_handle{
        fd,
        sys::poll_event::in,
        [fd, &events_handler](auto&&) {
            array<sys::event, 8>                  buff;
            fixed_vector<sys::event, buff.size()> filtered;

            if (auto count = sys::read(fd, buff)) {
                for (auto&& event : std::span{buff}.subspan(0, *count)) {
                    event.dispatch([&](keycodes, i32) {
                        filtered.emplace_back(event);
                    });
                }
                if (!filtered.empty())
                    events_handler(filtered);
            }


            if (auto count = sys::read(fd, buff))
                events_handler(std::span{buff}.subspan(0, *count));
        },
    };
}

class event_streamer {
public:
    event_streamer(const main_cmd<>& args):
        src{net::ip_addr_v4::any(), *args.src_port.get()}, dst(udp_addr_helper(*args.dst)), sock{src, false} {
        }

    void operator()(std::span<const sys::event> events) const {
        sock.send(dst, events);
    }

private:
    static std::string udp_addr_helper(const std::string& addr) {
        if (addr.starts_with("udp://"))
            return addr.substr(6);
        return addr;
    }

private:
    net::full_addr_v4 src;
    net::full_addr_v4 dst;
    net::udp_socket   sock;
};

class event_writter {
public:
    event_writter(const std::string& out_file): o{io::file::open(out_file, io::openflags::write_only | io::openflags::trunc | io::openflags::create, io::file_perms::o644)} {}

    void operator()(std::span<const sys::event> events) {
        o.write(events);
    }

private:
    io::out<io::file, 0> o;
};

void run(const main_cmd<>& args, auto&& streamer) {
    auto mouse = io::file::open(*args.mouse, sys::openflags::read_only | sys::openflags::nonblock);

    if (!args.kbd.get())
        poller{handle_mouse(mouse, streamer)}.run();
    else {
        auto kbd = io::file::open(*args.kbd.get(), sys::openflags::read_only | sys::openflags::nonblock);
        poller{handle_mouse(mouse, streamer), handle_keyboard(kbd, streamer)}.run();
    }
}

void tbc_main(main_cmd<> args) {
    auto dest = *args.dst;

    if (dest.starts_with("file://"))
        run(args, event_writter(dest.substr(7)));
    else
        run(args, event_streamer(args));
}

#include <util/tbc_main.hpp>

