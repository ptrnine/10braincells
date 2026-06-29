#define CORO_METAINFO

#include <core/async/main.hpp>
#include <core/async/sys/statx.hpp>
#include <core/async/sys/inotify_watch.hpp>
#include <core/async/util/recursive_dir_watch.hpp>

using namespace core;

task<> poll(async::inotify_watch::event_poller poller) {
    while (co_await poller.wait()) {
        for (auto event : poller.poll_view()) {
            glog().warn("{} {}", event.name, event.flags.to_string());
        }
    }
}

task<int> async_main(std::span<char*> args) try {
    const char* path = "./";
    if (args.size() > 1) {
        path = args[1];
    }

    async::util::recursive_dir_watch rdw{{}, path, sys::inotify_watch_flag::create | sys::inotify_watch_flag::close_write | sys::inotify_watch_flag::modify};

    while (auto events = co_await rdw.poll()) {
        for (auto& e : *events) {
            glog().warn("EVENT: {} {}", e.name, e.flags.to_string());
        }
    }

    co_return 0;
} catch (const std::exception& e) {
    glog().error("Exception in main: {}", e.what());
    co_return 1;
}
