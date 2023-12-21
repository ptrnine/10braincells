#pragma once

#include <core/basic_types.hpp>

namespace core {
enum class io_read_rc {
    ok = 0,
    timeout,
    partial,
    error,
};

struct io_read_res {
    size_t actually_read;
    io_read_rc rc = io_read_rc::ok;
};

enum class fd_exception_on_syswrite_fail { disable = 0, enable };
enum class fd_exception_on_seek_fail { disable = 0, enable };
enum class fd_exception_on_read_fail { disable = 0, enable };
} // namespace core
