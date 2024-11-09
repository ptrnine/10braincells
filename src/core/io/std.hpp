#pragma once

#include <core/io/in.hpp>
#include <core/io/out.hpp>

namespace core::io
{
static in  std_in(stdin_fd);
static out std_out(stdout_fd);
static out std_err(stderr_fd, size_c<0>);
} // namespace core::io
