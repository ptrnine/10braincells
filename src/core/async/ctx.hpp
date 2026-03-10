#pragma once

#include <core/io/uring/ctx.hpp>

namespace core {
static inline thread_local io::uring::ctx* current_ctx = nullptr;
}
