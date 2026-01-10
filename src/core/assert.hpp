#pragma once

#include <core/exception.hpp>
#include <core/io/std.hpp>

namespace core::details {
inline void _assert(const char* what, bool cond, const char* file, int line, const char* function, const char* expr) {
    if (!cond) {
        io::std_err.write(
            std::string_view(what),
            std::string_view{expr},
            "\n  File: ",
            std::string_view{file},
            ":",
            std::to_string(line),
            "\n  Function: ",
            std::string_view{function},
            "\n"
        );
        std::terminate();
    }
}
} // namespace core

#define _TBC_r_assert_what(what, ...) \
    core::details::_assert(what, __VA_ARGS__, __FILE__, __LINE__, __PRETTY_FUNCTION__, #__VA_ARGS__)

#define tbc_r_assert(...) _TBC_r_assert_what("Assert failed: ", __VA_ARGS__)

#if defined(DEBUG)
#define tbc_assert(...) tbc_r_assert(__VA_ARGS__)
#define tbc_expects(...) _TBC_r_assert_what("Precondition failed: ", __VA_ARGS__)
#define tbc_ensures(...) _TBC_r_assert_what("Postcondition failed: ", __VA_ARGS__)
#else
#define tbc_assert(...) (void(0))
#define tbc_expects(...) (void(0))
#define tbc_ensures(...) (void(0))
#endif

