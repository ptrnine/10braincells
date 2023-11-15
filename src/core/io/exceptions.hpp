#pragma once

#include <core/errc.hpp>

#include <stdexcept>
#include <string>

namespace core {
class io_error : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class cannot_open_file : public std::runtime_error {
public:
    cannot_open_file(const std::string& filename, errc err):
        std::runtime_error("Cannot open file \"" + filename + "\": " + err.info()), error(err) {}
    errc error;
};

class stat_fd_failed : public std::runtime_error {
public:
    stat_fd_failed(const std::string& filename, errc err, int iunclosed_fd = -1):
        std::runtime_error("Failed to stat file \"" + filename + "\": " + err.info()), error(err),
        unclosed_fd(iunclosed_fd) {}
    errc error;
    int unclosed_fd;
};

class ftruncate_fd_failed : public std::runtime_error {
public:
    ftruncate_fd_failed(const std::string& filename, errc err):
        std::runtime_error("Failed to ftruncate file \"" + filename + "\": " + err.info()), error(err) {}
    errc error;
};

class mmap_fd_failed : public std::runtime_error {
public:
    mmap_fd_failed(const std::string& filename, errc err, int iunclosed_fd = -1):
        std::runtime_error("Failed to mmap file \"" + filename + "\": " + err.info()), error(err),
        unclosed_fd(iunclosed_fd) {}
    errc error;
    int unclosed_fd;
};

class sys_write_fail : public io_error {
public:
    sys_write_fail(const std::string& msg, errc err):
        io_error("write() syscall failed: " + msg + (err.code ? (": " + err.info()) : std::string{})), error(err) {}
    errc error;
};

class sys_read_fail : public io_error {
public:
    sys_read_fail(errc err): io_error("read() syscall failed: " + err.info()), error(err) {}
    errc error;
};

class sys_seek_fail : public io_error {
public:
    sys_seek_fail(errc err): io_error("lseek() syscall failed: " + err.info()), error(err) {}
    errc error;
};
} // namespace core
