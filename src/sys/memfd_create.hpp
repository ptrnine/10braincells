#pragma once

#include <core/concepts/string.hpp>

#include <sys/syscall.hpp>
#include <sys/memfd_flags.hpp>

#define fwd(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace sys
{
template <typename... Ts>
    requires(core::null_term_string<Ts> || ... || false)
inline auto memfd_create(Ts&&... args) {
    const char* name;
    memfd_flags flags = memfd_flag{0};

    core::fwd_as_tuple(fwd(args)...)
        .foreach (core::overloaded{
            [&](const std::string& v) {
                name = v.data();
            },
            [&](const char* v) {
                name = v;
            },
            [&](memfd_flags v) {
                flags |= v;
            },
        });

    return syscall<fd_t>(SYS_memfd_create, name, flags.value);
}
} // namespace sys

#undef fwd
