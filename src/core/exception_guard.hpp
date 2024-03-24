#pragma once

#include <core/exception.hpp>
#include <core/utility/move.hpp>

namespace core
{
template <typename F>
struct exception_guard {
    exception_guard(F ifinalizer): finalizer(mov(ifinalizer)) {}
    ~exception_guard() {
        if (!dismissed && uncaught_exceptions() != uncaught)
            finalizer();
    }

    void dismiss() {
        dismissed = true;
    }

    F            finalizer;
    unsigned int uncaught  = uncaught_exceptions();
    bool         dismissed = false;
};
} // namespace core
