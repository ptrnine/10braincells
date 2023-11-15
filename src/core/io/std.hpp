#include "core/io/types.hpp"
#include <core/io.hpp>

namespace core {
inline auto stdout() {
    return outfd<char, 8192, fd_exception_on_seek_fail::enable, fd_exception_on_syswrite_fail::enable>::stdout();
}
inline auto stderr() {
    return outfd<char, 0, fd_exception_on_seek_fail::enable, fd_exception_on_syswrite_fail::enable>::stderr();
}
inline auto stdin() {
    return infd<char, 8192, fd_exception_on_seek_fail::enable, fd_exception_on_read_fail::enable>::stdin();
}
} // namespace core
