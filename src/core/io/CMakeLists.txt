codegen(io_uring_setup_flags_cg
    PROGRAM cg_flags
    ARGS
        name=setup_flag name_combined=setup_flags type=u32 namespace_name=core::io::uring
        "output=${CMAKE_CURRENT_BINARY_DIR}/uring_setup_flags.cg.hpp"
        "iopoll             = 1U << 0"
        "sqpoll             = 1U << 1"
        "sq_aff             = 1U << 2"
        "cqsize             = 1U << 3"
        "clamp              = 1U << 4"
        "attach_wq          = 1U << 5"
        "r_disabled         = 1U << 6"
        "submit_all         = 1U << 7"
        "coop_taskrun       = 1U << 8"
        "taskrun_flag       = 1U << 9"
        "sqe128             = 1U << 10"
        "cqe32              = 1U << 11"
        "single_issuer      = 1U << 12"
        "defer_taskrun      = 1U << 13"
        "no_mmap            = 1U << 14"
        "registered_fd_only = 1U << 15"
        "no_sqarray         = 1U << 16"
        "hybrid_iopoll      = 1U << 17"
    OUTPUT
        "${CMAKE_CURRENT_BINARY_DIR}/uring_setup_flags.cg.hpp"
)

codegen(io_uring_sqe_flags_cg
    PROGRAM cg_flags
    ARGS
        name=sqe_flag name_combined=sqe_flags type=u8 namespace_name=core::io::uring
        "output=${CMAKE_CURRENT_BINARY_DIR}/uring_sqe_flags.cg.hpp"
        "fixed_file       = 1U << 0"
        "io_drain         = 1U << 1"
        "io_link          = 1U << 2"
        "io_hardlink      = 1U << 3"
        "async            = 1U << 4"
        "buffer_select    = 1U << 5"
        "cqe_skip_success = 1U << 6"
    OUTPUT
        "${CMAKE_CURRENT_BINARY_DIR}/uring_sqe_flags.cg.hpp"
)

codegen(io_uring_enter_flags_cg
    PROGRAM cg_flags
    ARGS
        name=enter_flag name_combined=enter_flags type=u8 namespace_name=core::io::uring
        "output=${CMAKE_CURRENT_BINARY_DIR}/uring_enter_flags.cg.hpp"
        "getevents       = 1U << 0"
        "sq_wakeup       = 1U << 1"
        "sq_wait         = 1U << 2"
        "ext_arg         = 1U << 3"
        "registered_ring = 1U << 4"
        "abs_timer       = 1U << 5"
        "ext_arg_reg     = 1U << 6"
    OUTPUT
        "${CMAKE_CURRENT_BINARY_DIR}/uring_enter_flags.cg.hpp"
)

codegen(io_uring_feat_flags_cg
    PROGRAM cg_flags
    ARGS
        name=feat_flag name_combined=feat_flags type=u32 namespace_name=core::io::uring
        "output=${CMAKE_CURRENT_BINARY_DIR}/uring_feat_flags.cg.hpp"
        "single_mmap     = 1U << 0"
        "nodrop          = 1U << 1"
        "submit_stable   = 1U << 2"
        "rw_cur_pos      = 1U << 3"
        "cur_personality = 1U << 4"
        "fast_poll       = 1U << 5"
        "poll_32bits     = 1U << 6"
        "sqpoll_nonfixed = 1U << 7"
        "ext_arg         = 1U << 8"
        "native_workers  = 1U << 9"
        "rsrc_tags       = 1U << 10"
        "cqe_skip        = 1U << 11"
        "linked_file     = 1U << 12"
        "reg_reg_ring    = 1U << 13"
        "recvsend_bundle = 1U << 14"
        "min_timeout     = 1U << 15"
    OUTPUT
        "${CMAKE_CURRENT_BINARY_DIR}/uring_feat_flags.cg.hpp"
)

add_custom_target(io_uring_cg
    DEPENDS
        io_uring_setup_flags_cg
        io_uring_sqe_flags_cg
        io_uring_enter_flags_cg
        io_uring_feat_flags_cg
)
