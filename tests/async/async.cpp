#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include <unistd.h>

#include <sys/sigprocmask.hpp>
#include <sys/syscall.hpp>

#include <core/aggregate_exception.hpp>
#include <core/async/concurrent.hpp>
#include <core/async/runner.hpp>
#include <core/async/sys/cancel.hpp>
#include <core/async/sys/close.hpp>
#include <core/async/sys/open.hpp>
#include <core/async/sys/read.hpp>
#include <core/async/sys/readdir.hpp>
#include <core/async/sys/sleep.hpp>
#include <core/async/sys/statx.hpp>
#include <core/async/sys/write.hpp>
#include <core/async/wait_all.hpp>
#include <core/exec.hpp>
#include <core/io/file.hpp>

using namespace core;

namespace {
// Each test runs in its own io_uring context. This mirrors what async_main in
// core/async/main.hpp sets up (blocked SIGINT/SIGTERM + signalfd for the
// runner's stop_by_signal task).
struct ctx_guard {
    io::file signalfd;

    ctx_guard() {
        glog().set_level(util::log_level::error);

        auto sigset = sys::sigset::empty();
        sigset.add(SIGINT, SIGTERM);
        sys::sigprocmask(sys::sigmask::block, sigset).throw_if_error();

        signalfd = io::file::signalfd(sigset, sys::sigfd_flag::close_exec);
        async::current_signalfd = &signalfd;
    }

    ~ctx_guard() {
        async::current_signalfd = nullptr;
    }
};

#define fwd(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)

template <typename Coro>
auto run_ctx(Coro&& coro) {
    ctx_guard guard;
    return async::run_io_ctx(fwd(coro));
}
#undef fwd

// NOTE: tasks in this project are eager - the body starts running as soon as
// the task is constructed and only suspends at the first real co_await. These
// helpers have no io_awaitables, so they finish completely at construction.
task<int> value_task(int value) {
    co_return value;
}

task<int> nested_task() {
    co_return (co_await value_task(7)) * 3;
}

// The throw happens after the first suspension point: GCC does not route
// exceptions thrown before the first co_await into promise::unhandled_exception
// (they escape at construction). Throwing after a co_await exercises the
// project's real exception path (unhandled_exception -> task::await_resume).
task<void> throwing_task() {
    co_await async::sleep(std::chrono::milliseconds(1));
    throw std::runtime_error("test exception");
}

task<int> delayed_value(int value, std::chrono::milliseconds delay) {
    co_await async::sleep(delay);
    co_return value;
}

std::string unique_name() {
    return std::filesystem::temp_directory_path().string() + "/tbc_async_test_" + std::to_string(::getpid());
}
} // namespace

TEST_CASE("task value propagation, nesting and move semantics") {
    using result_t = std::tuple<int, int, int, bool, int>;

    auto [v, nested, moved, src_empty_after_move, completed_value] = run_ctx([] -> task<result_t> {
        auto v      = co_await value_task(42);
        auto nested = co_await nested_task();

        // Awaiting an already completed task.
        auto completed = value_task(5);

        // Move constructor transfers the handle, the source becomes empty.
        auto   src       = value_task(6);
        auto   src_empty = false;
        {
            CHECK(!src.empty());
            auto dst = mov(src);
            src_empty = src.empty();
            auto moved = co_await dst;
            co_return std::tuple{v, nested, moved, src_empty, co_await completed};
        }
    });
    CHECK(v == 42);
    CHECK(nested == 21);
    CHECK(moved == 6);
    CHECK(src_empty_after_move);
    CHECK(completed_value == 5);
}

TEST_CASE("task exception propagation") {
    // Exception in a task is rethrown to the awaiter; if not caught in the
    // main task it propagates out of run_io_ctx.
    CHECK_THROWS_WITH(run_ctx([] -> task<void> {
        co_await throwing_task();
    }),
                      "test exception");

    // ... and can be caught by the awaiter.
    bool caught = false;
    run_ctx([&] -> task<void> {
        try {
            co_await throwing_task();
        } catch (const std::runtime_error& e) {
            caught = std::string_view{e.what()} == "test exception";
        }
        co_return;
    });
    CHECK(caught);
}

TEST_CASE("async sleep") {
    auto [ok, elapsed_ms] = run_ctx([] -> task<std::tuple<bool, long long>> {
        auto t0       = std::chrono::steady_clock::now();
        auto res      = co_await async::sleep(std::chrono::milliseconds(5));
        auto elapsed  = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();
        co_return std::tuple{res.ok(), elapsed};
    });
    CHECK(ok);
    // Loose bounds to avoid flakes: the timer must fire, but it may be late.
    CHECK(elapsed_ms >= 4);
    CHECK(elapsed_ms < 5000);
}

TEST_CASE("async cancel of a pending sleep") {
    // Tasks are eager: async::sleep(...) has already submitted its io_uring
    // timeout at construction, so the cancel below deterministically hits it.
    auto [cancel_ok, sleep_err] = run_ctx([] -> task<std::tuple<bool, int>> {
        auto sleeper    = async::sleep(std::chrono::milliseconds(500));
        auto cancel_res = co_await sleeper.cancel();
        auto sleep_res  = co_await sleeper;
        co_return std::tuple{cancel_res.ok(), int(sleep_res.error())};
    });
    CHECK(cancel_ok);
    CHECK(sleep_err == int(errc::ecanceled));
}

TEST_CASE("async pipe write/read") {
    auto [read_back, written, eof] = run_ctx([] -> task<std::tuple<std::string, size_t, size_t>> {
        auto pipe = io::file::pipe();

        const char* const text = "hello async";
        auto written_res       = co_await async::write(pipe.out.fd(), text, std::char_traits<char>::length(text));
        written_res.throw_if_error();

        std::vector<char> buff(32);
        auto read_res     = co_await async::read(pipe.in.fd(), buff.data(), buff.size());
        read_res.throw_if_error();

        // Close the write side so the next read can observe EOF.
        co_await pipe.out.close_async();

        auto rest_res = co_await async::read(pipe.in.fd(), buff.data(), buff.size());
        rest_res.throw_if_error();

        co_return std::tuple{std::string(buff.data(), read_res.get()), written_res.get(), rest_res.get()};
    });
    CHECK(read_back == "hello async");
    CHECK(written == 11);
    CHECK(eof == 0);
}

TEST_CASE("async file open/statx/read/close") {
    auto path    = unique_name() + ".file";
    auto content = std::string("0123456789");
    {
        std::ofstream out(path, std::ios::binary);
        out << content;
    }

    auto [st_size, read_back] = run_ctx([path, content] -> task<std::tuple<u64, std::string>> {
        auto fd_res = co_await async::open(path, sys::openflags::read_only);
        fd_res.throw_if_error();
        auto fd = fd_res.get();

        auto st = (co_await async::statx(sys::statx_mask::all, path)).get();

        std::vector<char> buff(content.size());
        auto read_res     = co_await async::read(fd, buff.data(), buff.size());
        read_res.throw_if_error();

        co_await async::close(fd);

        co_return std::tuple{st.size, std::string(buff.data(), read_res.get())};
    });

    std::filesystem::remove(path);

    CHECK(st_size == content.size());
    CHECK(read_back == content);
}

TEST_CASE("async readdir") {
    auto dir    = unique_name() + ".dir";
    auto path_a = dir + "/a";
    auto path_b = dir + "/b";
    auto path_c = dir + "/c";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    std::ofstream(path_a, std::ios::binary) << "x";
    std::ofstream(path_b, std::ios::binary) << "y";
    std::ofstream(path_c, std::ios::binary) << "z";

    auto names = run_ctx([dir] -> task<std::set<std::string>> {
        auto fd_res = co_await async::open(dir, sys::openflags::read_only);
        fd_res.throw_if_error();

        std::set<std::string> names;
        auto                 gen = async::readdir<100>(fd_res.get());
        while (auto ent = co_await gen) {
            // getdents also yields the "." and ".." entries.
            if (ent->name != "." && ent->name != "..")
                names.insert(ent->name);
        }
        co_await async::close(fd_res.get());

        co_return names;
    });

    std::filesystem::remove_all(dir);

    CHECK(names.contains("a"));
    CHECK(names.contains("b"));
    CHECK(names.contains("c"));
    CHECK(names.size() == 3);
}

TEST_CASE("async wait_all") {
    auto [a, b] = run_ctx([] -> task<std::tuple<int, int>> {
        auto [x, y] = co_await async::wait_all(delayed_value(1, std::chrono::milliseconds(5)), delayed_value(2, std::chrono::milliseconds(1)));
        co_return std::tuple{x, y};
    });
    CHECK(a == 1);
    CHECK(b == 2);

    // A single exception inside wait_all is rethrown as-is.
    CHECK_THROWS_WITH(run_ctx([] -> task<void> {
        co_await async::wait_all(delayed_value(1, std::chrono::milliseconds(1)), throwing_task());
    }),
                      "test exception");

    // Multiple exceptions are bundled into an aggregate_exception.
    CHECK_THROWS_AS(run_ctx([] -> task<void> {
        co_await async::wait_all(throwing_task(), throwing_task());
    }),
                    aggregate_exception);
}

TEST_CASE("async concurrent select and push") {
    auto [first, second, third] = run_ctx([] -> task<std::tuple<int, int, int>> {
        // concurrent starts all tasks in the background immediately
        // (eager tasks + run_background started in the constructor).
        async::concurrent<int, 3> conc{delayed_value(20, std::chrono::milliseconds(2))};
        conc.push(delayed_value(10, std::chrono::milliseconds(5)));
        conc.push(delayed_value(30, std::chrono::milliseconds(12)));

        int first  = 0;
        int second = 0;
        int third  = 0;

        while (auto task = co_await conc.select()) {
            int value = co_await *task;
            if (first == 0) {
                first = value;
            } else if (second == 0) {
                second = value;
            } else {
                third = value;
            }
        }

        co_return std::tuple{first, second, third};
    });
    // Order is stable: 2ms < 5ms < 12ms, wide enough margins against flakes.
    CHECK(first == 20);
    CHECK(second == 10);
    CHECK(third == 30);
}

TEST_CASE("async process exec (fork/exec/waitid)") {
    auto [out, code] = run_ctx([] -> task<std::tuple<std::string, int>> {
        auto res = co_await async::exec(std::vector<std::string>{"/bin/sh", "-c", "printf hello; exit 3"});
        int code = 0;
        visit(
            res.code,
            overloaded{
                [&](process::exit_code ec) { code = ec.num; },
                [&](process::signal sig) { code = 1000 + sig.num; }
            }
        );
        co_return std::tuple{res.std_out, code};
    });
    CHECK(out == "hello");
    CHECK(code == 3);
}

