#pragma once

#include <core/concepts/string.hpp>
#include <sys/syscall.hpp>

namespace sys {
inline auto execve(const core::null_term_string auto& path, const char* const* args, const char* const* env) {
    constexpr auto to_data = core::overloaded{[](const char* s) { return s; }, [](const std::string& s) { return s.data(); }};
    return syscall<int>(SYS_execve, to_data(path), args, env);
}
} // namespace sys
