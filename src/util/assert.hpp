#pragma once

#include <core/macros.hpp>

#include "log.hpp"

#define R_ASSERT(...)                                                                                                  \
    util::assert_impl(__FILE_NAME__, __PRETTY_FUNCTION__, __LINE__, TBC_STRINGIFY(__VA_ARGS__), __VA_ARGS__)

#ifdef DEBUG
#define ASSERT(...) R_ASSERT(__VA_ARGS__)
#else
#define ASSERT(...) ((void)0)
#endif

namespace util
{
inline void
assert_impl(std::string_view file, std::string_view pretty_func, int line, std::string_view strexp, bool expr, auto&&... message) {
    if (!expr) {
        if constexpr (sizeof...(message) >= 1)
            glog().error("Assert failed: ({}) in [{}:{}] {}: {}\n",
                         strexp,
                         file,
                         line,
                         pretty_func,
                         format(static_cast<decltype(message)>(message)...));
        else
            glog().error("Assert failed: ({}) in [{}:{}] {}\n", strexp, file, line, pretty_func);
        std::terminate();
    }
}
} // namespace util
