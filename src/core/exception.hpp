#pragma once

#if 0
#if !defined(__EXCEPTION__) && !defined(__EXCEPTION_H)
namespace std {
int uncaught_exceptions() noexcept __attribute__((__pure__));

class exception /* NOLINT */ {
public:
    virtual ~exception() noexcept;
    virtual const char* what() const noexcept;
};
} // namespace std
#endif
#endif

#include <exception>

namespace core {
inline unsigned int uncaught_exceptions() noexcept {
    return static_cast<unsigned int>(std::uncaught_exceptions());
}

using exception = std::exception;
} // namespace core
