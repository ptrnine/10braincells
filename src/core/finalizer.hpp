#pragma once

#include <core/nostd/utility/move.hpp>

namespace core
{
template <typename F>
struct finalizer {
    finalizer(F ifinalizer): finalizer_f(move(ifinalizer)) {}
    ~finalizer() {
        if (!dismissed)
            finalizer_f();
    }

    void dismiss() {
        dismissed = true;
    }

    F    finalizer_f;
    bool dismissed = false;
};
} // namespace core
