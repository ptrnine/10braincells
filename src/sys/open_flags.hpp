#pragma once

#include <fcntl.h>

#include <string>

#include <core/array.hpp>
#include <core/tuple.hpp>

namespace sys {
/*
 * [[[codegen start]]]
 * Generated with:
 * ./build-deb/codegen/flags openflag openflags int read_only=O_RDONLY write_only=O_WRONLY read_write=O_RDWR
 * append=O_APPEND async=O_ASYNC close_exec=O_CLOEXEC create=O_CREAT direct=O_DIRECT dsync=O_DSYNC sync=O_SYNC
 * ensure_new=O_EXCL|O_CREAT large=O_LARGEFILE no_access_time=O_NOATIME no_follow=O_NOFOLLOW nonblock=O_NONBLOCK
 * temp=O_TMPFILE trunc=O_TRUNC
 */

enum class openflag : int {
    read_only      = O_RDONLY,
    write_only     = O_WRONLY,
    read_write     = O_RDWR,
    append         = O_APPEND,
    async          = O_ASYNC,
    close_exec     = O_CLOEXEC,
    create         = O_CREAT,
    direct         = O_DIRECT,
    dsync          = O_DSYNC,
    sync           = O_SYNC,
    ensure_new     = O_EXCL | O_CREAT,
    large          = O_LARGEFILE,
    no_access_time = O_NOATIME,
    no_follow      = O_NOFOLLOW,
    nonblock       = O_NONBLOCK,
    temp           = O_TMPFILE,
    trunc          = O_TRUNC,
};

struct openflags {
    constexpr openflags() = default;
    constexpr openflags(openflag ivalue): value(int(ivalue)) {}

    constexpr openflags operator|(openflag flag) const {
        return openflag(value | int(flag));
    }

    constexpr openflags operator&(openflag flag) const {
        return openflag(value & int(flag));
    }

    constexpr openflags& operator|=(openflag flag) {
        value = int(value | int(flag));
        return *this;
    }

    constexpr bool test(openflags flags) const {
        return value & flags.value;
    }

    explicit constexpr operator bool() const {
        return value;
    }

    constexpr std::string to_string() const {
        constexpr core::array flags = {
            core::tuple{openflag::read_only, std::string_view("read_only")},
            core::tuple{openflag::write_only, std::string_view("write_only")},
            core::tuple{openflag::read_write, std::string_view("read_write")},
            core::tuple{openflag::append, std::string_view("append")},
            core::tuple{openflag::async, std::string_view("async")},
            core::tuple{openflag::close_exec, std::string_view("close_exec")},
            core::tuple{openflag::create, std::string_view("create")},
            core::tuple{openflag::direct, std::string_view("direct")},
            core::tuple{openflag::dsync, std::string_view("dsync")},
            core::tuple{openflag::sync, std::string_view("sync")},
            core::tuple{openflag::ensure_new, std::string_view("ensure_new")},
            core::tuple{openflag::large, std::string_view("large")},
            core::tuple{openflag::no_access_time, std::string_view("no_access_time")},
            core::tuple{openflag::no_follow, std::string_view("no_follow")},
            core::tuple{openflag::nonblock, std::string_view("nonblock")},
            core::tuple{openflag::temp, std::string_view("temp")},
            core::tuple{openflag::trunc, std::string_view("trunc")},
        };
        std::string res;
        for (auto&& [f, s] : flags) {
            if (int(value) & int(f)) {
                res.append(s);
                res.append(" | ");
            }
        }
        if (res.size() > 2)
            res.resize(res.size() - 2);
        return res;
    }

    int value;
};

inline constexpr openflags operator|(openflag lhs, openflag rhs) {
    return openflag(int(lhs) | int(rhs));
}
/* [[[codegen end]]] */

struct file_perms {
public:
    enum perms_e : u16 {
        none         = 0,
        user_read    = S_IRUSR,
        user_write   = S_IWUSR,
        user_exec    = S_IXUSR,
        user_id_bit  = S_ISUID,
        user_rw      = user_read | user_write,
        user_rwx     = user_read | user_write | user_exec,
        group_read   = S_IRGRP,
        group_write  = S_IWGRP,
        group_exec   = S_IXGRP,
        group_id_bit = S_ISGID,
        group_rw     = group_read | group_write,
        group_rwx    = group_read | group_write | group_exec,
        other_read   = S_IROTH,
        other_write  = S_IWOTH,
        other_exec   = S_IXOTH,
        sticky_bit   = S_ISVTX,
        other_rw     = other_read | other_write,
        other_rwx    = other_read | other_write | other_exec
    };

    struct perms_str {
        static constexpr size_t permissions_str_size = 9;

        [[nodiscard]]
        const char* begin() const {
            return data;
        }
        [[nodiscard]]
        const char* end() const {
            return data + permissions_str_size;
        }
        [[nodiscard]]
        constexpr size_t size() const noexcept {
            return permissions_str_size;
        }

        char data[permissions_str_size + 1] = "---------";
    };

    constexpr file_perms(perms_e permissions = none): data(permissions) {}

    [[nodiscard]]
    constexpr perms_str to_string() const {
        return {{
            data & user_read ? 'r' : '-',
            data & user_write ? 'w' : '-',
            "-xSs"[(data & user_exec ? 1 : 0) + (data & user_id_bit ? 2 : 0)],
            data & group_read ? 'r' : '-',
            data & group_write ? 'w' : '-',
            "-xSs"[(data & group_exec ? 1 : 0) + (data & group_id_bit ? 2 : 0)],
            data & other_read ? 'r' : '-',
            data & other_write ? 'w' : '-',
            "-xTt"[(data & other_exec ? 1 : 0) + (data & sticky_bit ? 2 : 0)],
        }};
    }

    constexpr file_perms& operator|=(file_perms rhs) {
        data |= rhs.data;
        return *this;
    }

    constexpr file_perms& operator&=(file_perms rhs) {
        data &= rhs.data;
        return *this;
    }

    constexpr file_perms operator|(file_perms rhs) const {
        auto res = *this;
        res |= rhs;
        return res;
    }

    constexpr file_perms operator&(file_perms rhs) const {
        auto res = *this;
        res &= rhs;
        return res;
    }

    constexpr file_perms operator~() const {
        return perms_e(~data & u16(07777));
    }

    constexpr static file_perms from_number(int number) {
        return perms_e(number);
    }

    constexpr bool operator==(file_perms rhs) const {
        return data == rhs.data;
    }

    constexpr bool operator!=(file_perms rhs) const {
        return data != rhs.data;
    }

    constexpr explicit operator bool() const {
        return data;
    }

    constexpr bool operator!() const {
        return !data;
    }

    [[nodiscard]]
    constexpr int to_int() const {
        return data;
    }

private:
    u16 data;
};

constexpr inline file_perms operator|(file_perms::perms_e lhs, file_perms::perms_e rhs) {
    return file_perms(lhs) | rhs;
}

static inline constexpr fd_t fdcwd{AT_FDCWD};
static inline constexpr fd_t stdin_fd{0};
static inline constexpr fd_t stdout_fd{1};
static inline constexpr fd_t stderr_fd{2};
static inline constexpr fd_t invalid_fd{-1};
} // namespace sys
