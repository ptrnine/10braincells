#define CORO_METAINFO

#include <core/async/concurrent.hpp>
#include <core/async/main.hpp>
#include <core/async/sys/cancel.hpp>
#include <core/async/sys/inotify_watch.hpp>
#include <core/async/sys/readdir.hpp>
#include <core/async/sys/sleep.hpp>
#include <core/async/sys/statx.hpp>
#include <core/async/sys/waitid.hpp>
#include <core/io.hpp>
// #include <core/coro/read.hpp>

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
            glog().detail("  [{}]({}) {}", event.name, event.name.size(), event.flags.to_string());
        }
    }
}

task<void> destroy_after_10s(async::inotify_watch& watch) {
    (co_await async::sleep(std::chrono::seconds{10})).throw_if_error();
    watch = {};
}

task<void> test_concurrent() {
    async::concurrent<exec_result, 2> conc{
        async::exec({"/bin/bash", "-c", "sleep 2 && printf second"}),
        async::exec({"/bin/bash", "-c", "sleep 1 && printf first"}),
    };

    while (auto task = co_await conc.select()) {
        try {
            auto result = co_await *task;
            glog().detail("concurrent: {} {}", result.std_out, result.std_err);
        } catch (const std::exception& e) {
            glog().warn("concurrent: exception: {}", e.what());
        }
    }
}

task<void> test_concurrent2() {
    auto pipe1 = io::file::pipe();
    auto f1 = pipe1.in.fd();
    process proc1{
        arg::args = {"/bin/bash", "-c", "i=0; while [ $i -le 5 ]; do i=$((i+1)); printf 'proc first %s' $i; sleep 1; done; printf exit"},
        arg::std_out = mov(pipe1),
    };

    auto pipe2 = io::file::pipe();
    auto f2 = pipe2.in.fd();
    process proc2{
        arg::args = {"/bin/bash", "-c", "i=0; while [ $i -le 10 ]; do i=$((i+1)); printf 'proc second %s' $i; sleep 0.5; done; printf exit"},
        arg::std_out = mov(pipe2),
    };

    struct read_result {
        sys::fd_t   fd;
        std::string line;
    };
    struct exec_result {
        sys::pid_t      pid;
        process::result res;
    };
    using result_t = var<read_result, exec_result>;
    auto read_line = [](sys::fd_t fd) -> task<result_t> { co_return read_result{.fd = fd, .line = co_await async::read_all(fd)}; };
    auto exec      = [](process proc) -> task<result_t> {
        auto p = proc.run_async();
        co_return exec_result{.pid = *proc.pid(), .res = co_await p};
    };

    async::concurrent<result_t, 4> conc{};
    conc.push(exec(mov(proc1)));
    conc.push(exec(mov(proc2)));
    conc.push(read_line(f1));
    conc.push(read_line(f2));

    while (auto task = co_await conc.select()) {
        try {
            auto result = co_await *task;
            visit(
                result,
                overloaded{
                    [&](const read_result& res) {
                        if (!res.line.empty()) {
                            conc.push(read_line(res.fd));
                            glog().detail("concurrent: fd {} output: {}", int(res.fd), res.line);
                        }
                    },
                    [](const exec_result& res) {
                        visit(
                            res.res,
                            overloaded{
                                [&](process::exit_code ec) { glog().info("concurrent: process {} exited with {}", res.pid, ec.num); },
                                [&](process::signal sig) { glog().info("concurrent: process {} killed by signal {}", res.pid, sig.num); },
                            }
                        );
                    },
                }
            );
        } catch (const std::exception& e) {
            glog().warn("concurrent: exception: {}", e.what());
        }
    }
}

task<void> test_async() {
    std::vector<task<>> tasks;

    glog().detail("path: {}", realpath("./").get());

    async::inotify_watch watch{"./", sys::inotify_watch_flags::all_events};

    for (size_t i = 0; i < 10; ++i) {
        tasks.push_back(poll_ino_events(watch));
        tasks.push_back(poll_ino_events(watch));
        tasks.push_back(destroy_after_10s(watch));
        tasks.push_back(statx_test());
        tasks.push_back(test());
        tasks.push_back(proc());
        tasks.push_back(proc2());
        tasks.push_back(gen1());
        tasks.push_back(test_concurrent());
        tasks.push_back(test_concurrent2());
    }

    for (auto& t : tasks)
        co_await t;
}

task<int> async_main(std::span<char*>) try {
    auto f1 = async::async_run_io_ctx([] -> task<void> {
        auto t1 = async::async_run_io_ctx(test_async);
        auto t2 = test_async();
        co_await t1;
        co_await t2;
    });
    auto f2 = async::async_run_io_ctx(test_async);
    auto f3 = test_async();
    async::run_io_ctx(test_async);

    co_await async::wait_all(f1, f2, f3);
    co_return 0;
} catch (const std::exception& e) {
    glog().error("Exception in main: {}", e.what());
    co_return 1;
}
