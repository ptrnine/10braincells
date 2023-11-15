#include "core/io/types.hpp"
#include <core/io.hpp>

namespace core {
static auto std_out = outfd<char, 8192, fd_exception_on_seek_fail::enable, fd_exception_on_syswrite_fail::enable>::stdout();
static auto std_err = outfd<char, 0, fd_exception_on_seek_fail::enable, fd_exception_on_syswrite_fail::enable>::stderr();
static auto std_in = infd<char, 8192, fd_exception_on_seek_fail::enable, fd_exception_on_read_fail::enable>::stdin();

static auto std_outb = outfd<byte, 8192, fd_exception_on_seek_fail::enable, fd_exception_on_syswrite_fail::enable>::stdout();
static auto std_errb = outfd<byte, 0, fd_exception_on_seek_fail::enable, fd_exception_on_syswrite_fail::enable>::stderr();
static auto std_inb = infd<byte, 8192, fd_exception_on_seek_fail::enable, fd_exception_on_read_fail::enable>::stdin();
} // namespace core
