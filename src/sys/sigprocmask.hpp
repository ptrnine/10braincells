#pragma once

#include <sys/signal.hpp>
#include <sys/syscall.hpp>

namespace sys {
enum class sigmask {
    block   = SIG_BLOCK,
    unblock = SIG_UNBLOCK,
    setmask = SIG_SETMASK,
};

inline syscall_result<sigset> sigprocmask(sigmask how, sigset new_mask) {
    /* Cleanup SIGCANCEL and SIGSETXID */
    if (new_mask.test(__SIGRTMIN) || new_mask.test(__SIGRTMIN + 1))
        new_mask.del(__SIGRTMIN, __SIGRTMIN + 1);

    sigset old_mask;
    auto res = syscall<void>(SYS_rt_sigprocmask, how, &new_mask, &old_mask, _NSIG / 8);
    if (res)
        return syscall_result<sigset>::make_value(old_mask);
    else
        return res.error();
}
} // namespace sys
