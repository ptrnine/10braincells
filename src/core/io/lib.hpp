#pragma once

#include <core/io/in.hpp>
#include <core/io/out.hpp>

namespace core {
std::string read_all(sys::fd_t fd) {
    io::out o{std::string{}, size_c<0>};
    io::in{fd} >> o;
    return o.base_buff();
}

#ifndef DISABLE_ASYNC
namespace async {
    task<std::string> read_all(sys::fd_t fd) {
        io::out o{std::string{}, size_c<0>};
        io::in  i{fd, size_c<0>};

        std::string result;
        char        buff[4096];
        while (true) {
            auto sz = co_await i.read_async(std::span{buff, sizeof(buff)});
            if (sz != 0) {
                result.append(buff, sz);
            }
            if (sz != sizeof(buff)) {
                break;
            }
        }

        co_return result;
    }
} // namespace async
#endif
} // namespace core
