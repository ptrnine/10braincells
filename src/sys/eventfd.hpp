#pragma once

#include <sys/syscall.hpp>
#include <sys/eventfd_flags.hpp>

namespace sys {
syscall_result<fd_t> eventfd(u32 init, eventfd_flags flags = {}) {
    return syscall<fd_t>(SYS_eventfd2, init, flags.value);
}
} // namespace sys
