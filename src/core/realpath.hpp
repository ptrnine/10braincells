#pragma once

#include <core/concepts/string.hpp>
#include <core/constants.hpp>
#include <core/meta/type.hpp>
#include <core/traits/decay.hpp>
#include <sys/syscall.hpp>

extern "C" {
char* realpath(const char* filename, char* resolved);
}

namespace core {
template <typename T> requires null_term_string<decay<T>>
sys::syscall_result<std::string> realpath(const T& path) {
    const char* filename;
    if constexpr (type<T> == type<std::string>)
        filename = path.data();
    else
        filename = path;

    std::string resolved;
    resolved.resize(constants::path_max - 1);
    if (::realpath(filename, resolved.data())) {
        return sys::syscall_result<std::string>::make_value(mov(resolved));
    }
    return sys::syscall_result<std::string>::make_error(errc::from_errno());
}
} // namespace core
