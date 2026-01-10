#pragma once

#include <core/opt.hpp>
#include <core/var.hpp>

#include <sys/syscall.hpp>

namespace sys {
union sigval_t {
    int   val_int;
    void* val_ptr;
};

static constexpr auto siginfo_max_size = 128;
static constexpr auto siginfo_pad_size = __WORDSIZE == 64 ? (siginfo_max_size / sizeof(int)) - 4 : (siginfo_max_size / sizeof(int)) - 3;

// NOLINTNEXTLINE(performance-enum-size)
enum class child_status {
    exited    = 1, /* child has exited */
    killed    = 2, /* child was killed */
    dumped    = 3, /* child terminated abnormally */
    trapped   = 4, /* traced child has trapped */
    stopped   = 5, /* child has stopped */
    continued = 6, /* stopped child has continued */
};

struct siginfo_t {
    int          si_signo; /* Signal number.  */
    int          si_errno;
    child_status code;
#if __WORDSIZE == 64
    int __pad0; /* Explicit padding.  */
#endif
    union {
        int _pad[siginfo_pad_size];

        /* kill().  */
        struct {
            __pid_t si_pid; /* Sending process ID.  */
            __uid_t si_uid; /* Real user ID of sending process.  */
        } _kill;

        /* POSIX.1b timers.  */
        struct {
            int      si_tid;     /* Timer ID.  */
            int      si_overrun; /* Overrun count.  */
            sigval_t si_sigval;  /* Signal value.  */
        } _timer;

        /* POSIX.1b signals.  */
        struct {
            __pid_t  si_pid;    /* Sending process ID.  */
            __uid_t  si_uid;    /* Real user ID of sending process.  */
            sigval_t si_sigval; /* Signal value.  */
        } _rt;

        /* SIGCHLD.  */
        struct {
            __pid_t          si_pid;    /* Which child.	 */
            __uid_t          si_uid;    /* Real user ID of sending process.  */
            int              si_status; /* Exit value or signal.  */
            __kernel_clock_t si_utime;
            __kernel_clock_t si_stime;
        } _sigchld;

        /* SIGILL, SIGFPE, SIGSEGV, SIGBUS.  */
        struct {
            void*     si_addr;     /* Faulting insn/memory ref.  */
            short int si_addr_lsb; /* Valid LSB of the reported address.  */
            union {
                /* used when si_code=SEGV_BNDERR */
                struct {
                    void* _lower;
                    void* _upper;
                } _addr_bnd;
                /* used when si_code=SEGV_PKUERR */
                __uint32_t _pkey;
            } _bounds;
        } _sigfault;

        /* SIGPOLL.  */
        struct {
            long si_band; /* Band event for SIGPOLL.  */
            int  si_fd;
        } _sigpoll;
    } _sifields;

    [[nodiscard]]
    constexpr bool is_exited() const {
        return code == child_status::exited;
    }

    [[nodiscard]]
    constexpr int status() const {
        // NOLINTNEXTLINE
        return _sifields._sigchld.si_status;
    }
};
} // namespace sys
