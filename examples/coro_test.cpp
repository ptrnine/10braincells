#include <core/async/main.hpp>
#include <core/async/sleep.hpp>
#include <core/async/waitid.hpp>
#include <core/async/readdir.hpp>
//#include <core/coro/read.hpp>

#include <core/exec.hpp>

#include <sys/pipe.hpp>
#include <util/log.hpp>

using namespace core;

task<> test() {
    auto s1 = co_await async::sleep(std::chrono::seconds(2));
    util::glog().warn("Result: {}", s1.error().info());

    auto file = co_await io::file::open_async("./CMakeLists.txt", io::openflags::read_only);
    auto in = io::in{file};

    std::vector<char> test(13333);
    auto r = co_await in.read_async(test);
    util::glog().warn("Result: {}", r);

    co_await file.close_async();
}

task<> proc() {
    auto    pipe = io::file::pipe();
    io::in  in{pipe.in, size_c<0>};
    process proc{
        arg::args    = {"/bin/bash", "-c", "i=0; while [ $i -le 5 ]; do i=$((i+1)); printf hellloo; sleep 0.5; done; printf exit"},
        arg::std_out = mov(pipe),
    };

    auto p = proc.run_async();

    while (true) {
        std::vector<char> data(1024);
        auto              s = co_await in.read_async(data);
        if (s == 0)
            break;
        util::glog().warn("{}", data.data());
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

    util::glog().warn("{} {}", out.size(), out);
}

task<> gen1() {
    auto dirs = async::readdir<100>(io::file::open("./", sys::openflags::read_only | sys::openflags::directory));
    while (auto ent = co_await dirs) {
        util::glog().warn("file: {}", ent->name);
    }
}

task<int> coro_main(std::span<char*>) {
    std::vector<task<>> tasks;

    for (size_t i = 0; i < 1; ++i) {
        tasks.push_back(test());
        tasks.push_back(proc());
        tasks.push_back(proc2());
        tasks.push_back(gen1());
    }

    for (auto& t : tasks)
        co_await t;

    co_return 0;
}
