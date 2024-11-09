#pragma once

#include <core/io/basic_types.hpp>
#include <sys/statx.hpp>

namespace core::io
{
inline bool is_pipe_like(fd_t fd) {
    if (auto stx = sys::statx(fd, sys::statx_mask::size)) {
        switch (stx->mode.type()) {
        case sys::file_type::socket:
        case sys::file_type::fifo:
        case sys::file_type::chardev: return true;
        default: return false;
        }
    }
    return false;
}
} // namespace core::io
