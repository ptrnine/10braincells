#pragma once

#include <util/inotify.hpp>

class engine_object;

namespace details
{
static engine_object* engine_ptr = nullptr;
}

class engine_object {
public:
    engine_object() {
        details::engine_ptr = this;
    }

    ~engine_object() {
        details::engine_ptr = nullptr;
    }

    util::inotify_watcher dir_watcher;
};

inline engine_object& engine() {
    return *details::engine_ptr;
}
