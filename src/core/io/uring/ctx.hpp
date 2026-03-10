#pragma once
#include <core/moveonly_trivial.hpp>
#include <liburing.h>

#include <core/errc_exception.hpp>
#include <core/io/uring/structs.hpp>
#include <core/opt.hpp>
#include <core/coro/awaitable.hpp>
#include <sys/close.hpp>
#include <sys/open_flags.hpp>
#include <sys/eventfd.hpp>
#include <sys/poll.hpp>
#include <unordered_map>
//#include <util/log.hpp>

//#include <sys/waitid.hpp>

namespace core::io::uring {
class uring_exception : public errc_exception {
public:
    using errc_exception::errc_exception;
};

class sq_is_full : public uring_exception {
public:
    using uring_exception::uring_exception;
};

using uring_awaitable = awaitable_base<long>;

namespace masks {
    enum uring_data_masks : u64 {
        thread_task = 1LLU << 63,
    };
}

class ctx {
public:
    struct thread_task {
        thread_task(sys::fd_t fd, uring_awaitable* iawaitable): eventfd(fd), awaitable(iawaitable) {}

        ~thread_task() {
            if (eventfd.not_default()) {
                sys::close(eventfd).throw_if_error();
            }
        }

        thread_task(thread_task&&) = default;
        thread_task& operator=(thread_task&& rhs) noexcept {
            if (eventfd.not_default()) {
                sys::close(eventfd).throw_if_error();
            }
            eventfd   = mov(rhs.eventfd);
            awaitable = rhs.awaitable;

            return *this;
        }

        moveonly_trivial<sys::fd_t, sys::invalid_fd> eventfd;
        uring_awaitable*                             awaitable;
    };

    explicit ctx(unsigned entries, setup_flags flags = {}): ring(init) {
        int rc = io_uring_queue_init(entries, &*ring, flags.value);
        if (rc < 0)
            throw uring_exception{-rc};
    }

    ~ctx() {
        if (ring)
            io_uring_queue_exit(&*ring);
    }

    ctx(ctx&&) noexcept                 = default;
    ctx& operator=(ctx&&) noexcept      = default;
    ctx(const ctx&) noexcept            = delete;
    ctx& operator=(const ctx&) noexcept = delete;

    static void msec_to_ts(__kernel_timespec* ts, unsigned int msec) {
        ts->tv_sec  = msec / 1000;
        ts->tv_nsec = (msec % 1000) * 1000000;
    }

    auto sleep(__kernel_timespec& time) {
        return make_awaitable<int>([this, &time](awaitable_base<int>& awaitable) {
            auto& sqe = get_sqe();
            io_uring_prep_timeout(&sqe, &time, 0, 0);
            io_uring_sqe_set_data(&sqe, &awaitable);
            io_uring_submit(&*ring);
            // io_uring_submit_and_wait(&*ring, 1);
            // handle_cq();
        });
    }

    //awaitable waitid(sys::wait_type idtype, sys::fd_t id, sys::siginfo_t& siginfo, sys::wait_flags options) {
    //    auto sqe = get_sqe();
    //    if (!sqe) {
    //        throw std::runtime_error("SUKA");
    //    }

    //    io_uring_prep_waitid(sqe, idtype_t(idtype), id_t(id), (siginfo_t*)&siginfo, options.value, 0);
    //    return {&*ring, sqe};
    //    // sqe->user_data = (u64)h.
    //    // io_uring_submit((io_uring*)&ring);
    //}

    void handle_cq() {
        io_uring_cqe* cqe = nullptr;
        unsigned head = 0;
        //unsigned i = 0;
        io_uring_for_each_cqe(&*ring, head, cqe) {
            if (cqe->user_data & masks::thread_task) {
                cqe->user_data &= ~masks::thread_task;
                auto  efd       = sys::fd_t(cqe->user_data);
                auto  task_it   = thread_tasks.find(efd);
                auto* awaitable = task_it->second.awaitable;
                awaitable->resume(cqe->res);
                thread_tasks.erase(task_it);
            } else {
                auto* awaitable = to_awaitable_ptr<long>(cqe->user_data);
                awaitable->resume(cqe->res);
            }
            //io_uring_cqe_seen(&*ring, cqe);
            //util::glog().warn("[handle cq] next...");
            //std::coroutine_handle<>::from_address((void*)cqe->user_data).resume();
        }

        io_uring_smp_store_release(ring->cq.khead, head);
    }

    void exit(int code = 0) {
        _exit = code;
    }

    int run() {
        while (!_exit) {
            io_uring_cqe* cqe;
            auto rc = io_uring_wait_cqe(&*ring, &cqe);
            //int rc = io_uring_wait_cqe(&*ring, 1);
            if (rc < 0 && rc != -EINTR)
                throw uring_exception{-rc};
            handle_cq();
        }

        return *_exit;
    }

    io_uring_sqe& get_sqe() {
        auto sqe = try_get_sqe();
        if (!sqe) {
            throw sq_is_full{errc::exfull};
        }
        return *sqe;
    }

    io_uring* get_ring() {
        return ring ? &*ring : nullptr;
    }

    void schedule_thread_task(uring_awaitable& awaitable, auto&& launcher) {
        auto& sqe = get_sqe();
        auto efd = sys::eventfd(0, sys::eventfd_flags::nonblock).get();
        io_uring_prep_poll_add(&sqe, int(efd), unsigned(sys::poll_event::in));
        io_uring_sqe_set_data64(&sqe, u64(efd) | masks::thread_task);
        thread_tasks.emplace(efd, thread_task{efd, &awaitable});
        io_uring_submit(get_ring());
        launcher(efd);
    }

private:
    io_uring_sqe* try_get_sqe() {
        auto sqe = io_uring_get_sqe(&*ring);
        if (sqe) {
            sqe->user_data = 0;
        }
        return sqe;
    }

private:
    opt<io_uring> ring;
    opt<int>      _exit;

    std::unordered_map<sys::fd_t, thread_task> thread_tasks;
};
} // namespace core::io::uring
