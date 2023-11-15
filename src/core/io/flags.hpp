#pragma once

#include <fcntl.h>

#include <type_traits>

#include <core/base_types.hpp>

namespace core {
enum class fd_flag {
    read_only      = O_RDONLY,
    write_only     = O_WRONLY,
    readwrite      = O_RDWR,
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
    trunc          = O_TRUNC
};

struct fd_combined_flag {
    fd_combined_flag(fd_flag iflag): f(int(iflag)) {}
    fd_combined_flag operator|(fd_flag flag) const {
        return fd_flag(f | int(flag));
    }
    fd_combined_flag& operator|=(fd_flag flag) {
        f |= int(flag);
        return *this;
    }

    int f;
};

inline fd_combined_flag operator|(fd_flag lhs, fd_flag rhs) {
    using underlying_t = std::underlying_type_t<fd_flag>;
    return fd_flag(static_cast<underlying_t>(lhs) | static_cast<underlying_t>(rhs));
}

struct file_permissions {
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

    struct permissions_string {
        static constexpr size_t permissions_str_size = 9;

        const char* begin() const {
            return data;
        }
        const char* end() const {
            return data + permissions_str_size;
        }

        constexpr size_t size() const noexcept {
            return permissions_str_size;
        }

        char data[permissions_str_size + 1] = "---------";
    };

    constexpr file_permissions(perms_e permissions = none): data(permissions) {}

    constexpr permissions_string to_string() const {
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

    constexpr file_permissions& operator|=(file_permissions rhs) {
        data |= rhs.data;
        return *this;
    }

    constexpr file_permissions& operator&=(file_permissions rhs) {
        data &= rhs.data;
        return *this;
    }

    constexpr file_permissions operator|(file_permissions rhs) const {
        auto res = *this;
        res |= rhs;
        return res;
    }

    constexpr file_permissions operator&(file_permissions rhs) const {
        auto res = *this;
        res &= rhs;
        return res;
    }

    constexpr file_permissions operator~() const {
        return perms_e(~data & u16(07777));
    }

    constexpr static file_permissions from_number(int number) {
        return perms_e(number);
    }

    constexpr bool operator==(file_permissions rhs) const {
        return data == rhs.data;
    }

    constexpr bool operator!=(file_permissions rhs) const {
        return data != rhs.data;
    }

    constexpr explicit operator bool() const {
        return data;
    }

    constexpr bool operator!() const {
        return !data;
    }

    int to_int() const {
        return data;
    }

private:
    u16 data;
};

constexpr inline file_permissions operator|(file_permissions::perms_e lhs, file_permissions::perms_e rhs) {
    return file_permissions(lhs) | rhs;
}
} // namespace core
