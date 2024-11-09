#pragma once

#include <sys/basic_types.hpp>
#include <sys/lseek.hpp>
#include <sys/map_flags.hpp>
#include <sys/map_prot.hpp>
#include <sys/memfd_flags.hpp>
#include <sys/open_flags.hpp>
#include <sys/pipe.hpp>

namespace core::io
{
using sys::fd_t;
using sys::file_perms;
using sys::openflags;
using sys::seek_whence;
using sys::pipeflags;
using sys::memfd_flags;
using sys::map_prots;
using sys::map_flags;

using sys::invalid_fd;
using sys::stdin_fd;
using sys::stdout_fd;
using sys::stderr_fd;
} // namespace core::io
