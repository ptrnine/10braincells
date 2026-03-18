#pragma once

#include <core/io/uring/ctx.hpp>

namespace core {
thread_local io::uring::ctx* current_ctx = nullptr;
}
