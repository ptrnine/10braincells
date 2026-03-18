#include <core/io/uring/ctx.hpp>
#define CORO_METAINFO

#include <core/async/main.hpp>
#include <core/async/sleep.hpp>
#include <core/async/waitid.hpp>
#include <core/async/readdir.hpp>
#include <core/async/statx.hpp>
#include <core/async/cancel.hpp>
#include <core/async/inotify_watch.hpp>
//#include <core/coro/read.hpp>

#include <core/exec.hpp>

#include <sys/pipe.hpp>
#include <util/log.hpp>
#include <core/realpath.hpp>

using namespace core;

task<> test() {
    auto s1 = co_await async::sleep(std::chrono::seconds(2));
    glog().detail("Sleep result: {}", s1.error().info());

    auto file = co_await io::file::open_async("./CMakeLists.txt", io::openflags::read_only);
    auto in = io::in{file};

    std::vector<char> test(13333);
    auto r = co_await in.read_async(test);
    glog().detail("Result: {}", r);

    co_await file.close_async();
}

task<> proc() {
    auto    pipe = io::file::pipe();
    io::in  in{pipe.in, size_c<0>};
    process proc{
        arg::args    = {"/bin/bash", "-c", "i=0; while [ $i -le 5 ]; do i=$((i+1)); printf hellloo; sleep 1; done; printf exit"},
        arg::std_out = mov(pipe),
    };

    auto p = proc.run_async();

    while (true) {
        std::vector<char> data(1024);
        auto              s = co_await in.read_async(data);
        if (s == 0)
            break;
        glog().detail("{}", data.data());
    }
    co_await p;
}

task<> proc2() {
    std::string out;
    process     proc{
        arg::args    = {"/bin/bash", "-c", "i=0; while [ $i -le 5 ]; do i=$((i+1)); printf hellloo; done; printf exit"},
        arg::std_out = out,
    };

    co_await proc.run_async();

    glog().detail("{} {}", out.size(), out);
}

task<> gen1() {
    auto dirs = async::readdir<100>(io::file::open("./", sys::openflags::read_only | sys::openflags::directory));
    while (auto ent = co_await dirs) {
        glog().detail("file: {}", ent->name);
    }
}

task<> statx_test() {
    auto res = (co_await async::statx(sys::statx_mask::all, "./CMakeLists.txt")).get();
    glog().detail("{} {}", res.ino, res.size);
}

task<void> poll_ino_events(async::inotify_watch& watch) {
    auto poller = watch.poller();
    while (co_await poller.wait()) {
        glog().detail("events:");
        for (auto event : poller.poll()) {
            glog().detail("  {} {}", event.name, event.flags.to_string());
        }
    }
}

task<void> destroy_after_10s(async::inotify_watch& watch) {
    co_await async::sleep(std::chrono::seconds{10});
    watch = {};
}

task<void> test_async() {
    std::vector<task<>> tasks;

    glog().detail("path: {}", realpath("./").get());

    async::inotify_watch watch{"./", sys::inotify_watch_flags::all_events};

    for (size_t i = 0; i < 1; ++i) {
        tasks.push_back(poll_ino_events(watch));
        tasks.push_back(poll_ino_events(watch));
        tasks.push_back(destroy_after_10s(watch));
        tasks.push_back(statx_test());
        tasks.push_back(test());
        tasks.push_back(proc());
        tasks.push_back(proc2());
        tasks.push_back(gen1());
    }

    for (auto& t : tasks)
        co_await t;
}

task<int> async_main(std::span<char*>) try {
    auto f1 = async::spawn_child([] -> task<void> {
        auto t1 = async::spawn_child(test_async);
        auto t2 = test_async();
        co_await t1;
        co_await t2;
    });
    auto f2 = async::spawn_child(test_async);
    auto f3 = test_async();
    async::run(test_async);

    co_await f1;
    co_await f2;
    co_await f3;
    co_return 0;
} catch (const std::exception& e) {
    glog().error("Exception in main: {}", e.what());
    co_return 1;
}
