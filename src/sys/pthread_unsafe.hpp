#pragma once

#include <atomic>
#include <cstddef>

#include <sys/basic_types.hpp>

namespace sys {
struct tcbhead {
    void*                  tcb;
    void*                  dtv;
    void*                  self;
    int                    multiple_threads;
    int                    gscope_flag;
    uptr              sysinfo;
    uptr              stack_guard;
    uptr              pointer_guard;
    unsigned long int      unused_vgetcpu_cache[2];
    unsigned int           feature_1;
    int                    __glibc_unused1;
    void*                  __private_tm[4];
    void*                  __private_ss;
    unsigned long long int ssp_base;
    int                    __glibc_unused2[128] __attribute__((aligned(32)));
    void*                  __padding[8];
};

struct list_t {
    void* next;
    void* prev;
};

struct robust_list_head {
    void*    list;
    long int futex_offset;
    void*    list_op_pending;
};

struct __attribute((aligned(32))) pthread_descr {
    union {
        tcbhead header;
        void*   padd[24];
    };
    list_t           list;
    pid_t            tid;
    void*            robust_prev;
    robust_list_head robust_head;
    void*            cleanup_buffer;
    void*            unwind_buffer;
    std::atomic_uint cancelhandling;
};

pthread_descr& get_pthread() {
    pthread_descr* pt_self;
    asm("mov %%fs:%c1,%0" : "=r"(pt_self) : "i"(offsetof(pthread_descr, header.self)));
    return *pt_self;
}

namespace canceling_bits
{
    enum : uint {
        state      = 1 << 0,
        async      = 1 << 1,
        canceling  = 1 << 2,
        canceled   = 1 << 3,
        exiting    = 1 << 4,
        terminated = 1 << 5,
    };
}

uint enable_async_cancel() {
    using namespace canceling_bits;

    auto& t      = get_pthread();
    auto& cancel = t.cancelhandling;
    auto  old    = cancel.load(std::memory_order_relaxed);

    while (true) {
        auto new_val = old | async;
        if (new_val == old)
            break;

        if (cancel.compare_exchange_weak(old, new_val, std::memory_order_acquire)) {
            if ((new_val & (state | async | canceled | exiting | terminated)) == (async | canceled))
                pthread_exit(PTHREAD_CANCELED);
            break;
        }
    }

    return old;
}

void disable_async_cancel(uint old) {
    using namespace canceling_bits;

    if (old & async)
        return;

    auto& cancel = get_pthread().cancelhandling;
    old          = cancel.load(std::memory_order_relaxed);

    uint new_val;
    do {
        new_val = old & ~canceling_bits::async;
    } while (!cancel.compare_exchange_weak(old, new_val, std::memory_order_acquire));

    while ((new_val & (canceled | canceling)) == canceling) [[unlikely]] {
        cancel.wait(new_val, std::memory_order_relaxed);
        new_val = cancel.load(std::memory_order_relaxed);
    }
}
} // namespace sys
