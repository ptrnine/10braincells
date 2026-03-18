#pragma once

#include <core/async/runner.hpp>
#include <core/io/file.hpp>

core::task<int> async_main(std::span<char*> args);

int main(int argc, char** argv) {
    auto sigset = sys::sigset::empty();
    sigset.add(SIGINT, SIGTERM);
    sys::sigprocmask(sys::sigmask::block, sigset).throw_if_error();
    auto signalfd = core::io::file::signalfd(sigset, sys::sigfd_flag::close_exec);

    return core::async::run([argc, argv] { return async_main(std::span{argv, size_t(argc)}); }, core::mov(signalfd));
}
