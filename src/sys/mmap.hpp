#pragma once

#include <sys/map_prot.hpp>
#include <sys/map_flags.hpp>
#include <sys/open_flags.hpp>
#include <sys/remap_flags.hpp>

#include <sys/syscall.hpp>

#define fwd(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace sys
{
struct map_offset {
    off_t value;
};

template <typename... Ts>
inline auto mmap(Ts&&... args) {
    map_offset off{0};
    map_flags  flags{};
    map_prots  prot{};
    void*      addr = nullptr;
    size_t     len  = 0;
    auto       fd   = invalid_fd;

    core::fwd_as_tuple(fwd(args)...)
        .foreach (core::overloaded{
            [&](void* v) {
                addr = v;
            },
            [&](size_t v) {
                len = v;
            },
            [&](map_prots v) {
                prot = v;
            },
            [&](map_flags v) {
                flags = v;
            },
            [&](fd_t v) {
                fd = v;
            },
            [&](map_offset v) {
                off = v;
            },
        });

    static constexpr auto mmap_of_mask = (-0x2000ULL << (8 * sizeof(sc_arg) - 1)) | (4096ULL - 1);
    if (u64(off.value) & mmap_of_mask)
        return syscall_result<void*>{errc::einval};
    if (len >= std::numeric_limits<std::ptrdiff_t>::max())
        return syscall_result<void*>{errc::einval};

    return syscall<void*>(SYS_mmap, addr, len, prot.value, flags.value, int(fd), off.value);
}

inline auto munmap(void* addr, size_t size) {
    return syscall<void>(SYS_munmap, addr, size);
}

inline auto mremap(void* old_addr, size_t old_len, size_t new_len, remap_flags flags = {}, void* new_addr = nullptr) {
    return syscall<void*>(SYS_mremap, old_addr, old_len, new_len, flags.value, new_addr);
}
} // namespace sys

#undef fwd
