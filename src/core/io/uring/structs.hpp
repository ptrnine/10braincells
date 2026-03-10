#pragma once

#include <core/io/uring/flags.hpp>

namespace core::io::uring {
struct sq_offsets {
    u32 head;
    u32 tail;
    u32 ring_mask;
    u32 ring_entries;
    u32 flags;
    u32 dropped;
    u32 array;
    u32 resv1;
    u64 user_addr;
};

struct cq_offsets {
    u32 head;
    u32 tail;
    u32 ring_mask;
    u32 ring_entries;
    u32 overflow;
    u32 cqes;
    u32 flags;
    u32 resv1;
    u64 user_addr;
};

struct uring_params {
    u32         sq_entries;
    u32         cq_entries;
    setup_flags flags;
    u32         sq_thread_cpu;
    u32         sq_thread_idle;
    feat_flags  features;
    sys::fd_t   wq_fd;
    u32         resv[3];
    sq_offsets  sq_off;
    cq_offsets  cq_off;
};

/*
 * IO submission data structure (Submission Queue Entry)
 */
struct sqe_t {
    u8        opcode; /* type of operation for this sqe */
    u8        flags;  /* IOSQE_ flags */
    u16       ioprio; /* ioprio for the request */
    sys::fd_t fd;     /* file descriptor to do IO on */
    union {
        u64 off; /* offset into file */
        u64 addr2;
        struct {
            u32 cmd_op;
            u32 __pad1;
        };
    };
    union {
        u64 addr; /* pointer to buffer or iovecs */
        u64 splice_off_in;
        struct {
            u32 level;
            u32 optname;
        };
    };
    u32 len; /* buffer size or number of iovecs */
    union {
        int rw_flags;
        u32 fsync_flags;
        u16 poll_events;   /* compatibility */
        u32 poll32_events; /* word-reversed for BE */
        u32 sync_range_flags;
        u32 msg_flags;
        u32 timeout_flags;
        u32 accept_flags;
        u32 cancel_flags;
        u32 open_flags;
        u32 statx_flags;
        u32 fadvise_advice;
        u32 splice_flags;
        u32 rename_flags;
        u32 unlink_flags;
        u32 hardlink_flags;
        u32 xattr_flags;
        u32 msg_ring_flags;
        u32 uring_cmd_flags;
        u32 waitid_flags;
        u32 futex_flags;
        u32 install_fd_flags;
        u32 nop_flags;
    };
    u64 user_data; /* data to be passed back at completion time */
    /* pack this to avoid bogus arm OABI complaints */
    union {
        /* index into fixed buffers, if used */
        u16 buf_index;
        /* for grouped buffer selection */
        u16 buf_group;
    } __attribute__((packed));
    /* personality to use, if used */
    u16 personality;
    union {
        i32 splice_fd_in;
        u32 file_index;
        u32 optlen;
        struct {
            u16 addr_len;
            u16 __pad3[1];
        };
    };
    union {
        struct {
            u64 addr3;
            u64 __pad2[1];
        };
        u64 optval;
        /*
         * If the ring is initialized with IORING_SETUP_SQE128, then
         * this field is used for 80 bytes of arbitrary command data
         */
        u8 cmd[0];
    };
};

struct sq_state {
    unsigned* khead;
    unsigned* ktail;
    // Deprecated: use `ring_mask` instead of `*kring_mask`
    unsigned* kring_mask;
    // Deprecated: use `ring_entries` instead of `*kring_entries`
    unsigned* kring_entries;
    unsigned* kflags;
    unsigned* kdropped;
    unsigned* array;
    sqe_t*    sqes;

    unsigned sqe_head;
    unsigned sqe_tail;

    size_t ring_sz;
    void*  ring_ptr;

    unsigned ring_mask;
    unsigned ring_entries;

    unsigned pad[2];
};

struct cq_state {
    unsigned* khead;
    unsigned* ktail;
    // Deprecated: use `ring_mask` instead of `*kring_mask`
    unsigned* kring_mask;
    // Deprecated: use `ring_entries` instead of `*kring_entries`
    unsigned*            kring_entries;
    unsigned*            kflags;
    unsigned*            koverflow;
    struct io_uring_cqe* cqes;

    size_t ring_sz;
    void*  ring_ptr;

    unsigned ring_mask;
    unsigned ring_entries;

    unsigned pad[2];
};

struct state {
    sq_state sq;
    cq_state cq;
    unsigned flags;
    int      ring_fd;

    unsigned features;
    int      enter_ring_fd;
    u8       int_flags;
    u8       pad[3];
    unsigned pad2;
};
} // namespace core::io::uring
