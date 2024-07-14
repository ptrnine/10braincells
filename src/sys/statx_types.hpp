#pragma once

#include <string>
#include <sys/stat.h>

#include <sys/chrono.hpp>
#include <sys/open_flags.hpp>

namespace sys
{
/*
 * [[[codegen start]]]
 * Generated with:
 * ./build-deb/codegen/flags statx_mask statx_masks u32 type=STATX_TYPE mode=STATX_MODE nlink=STATX_NLINK uid=STATX_UID
 * gid=STATX_GID atime=STATX_ATIME mtime=STATX_MTIME ctime=STATX_CTIME ino=STATX_INO size=STATX_SIZE blocks=STATX_BLOCKS
 * basic_stats=STATX_BASIC_STATS btime=STATX_BTIME all=STATX_ALL mnt_id=STATX_MNT_ID dio_mem_align=STATX_DIOALIGN
 */
enum class statx_mask : u32 {
    type          = STATX_TYPE,
    mode          = STATX_MODE,
    nlink         = STATX_NLINK,
    uid           = STATX_UID,
    gid           = STATX_GID,
    atime         = STATX_ATIME,
    mtime         = STATX_MTIME,
    ctime         = STATX_CTIME,
    ino           = STATX_INO,
    size          = STATX_SIZE,
    blocks        = STATX_BLOCKS,
    basic_stats   = STATX_BASIC_STATS,
    btime         = STATX_BTIME,
    all           = STATX_ALL,
    mnt_id        = STATX_MNT_ID,
    dio_mem_align = STATX_DIOALIGN,
};

struct statx_masks {
    constexpr statx_masks() = default;
    constexpr statx_masks(statx_mask ivalue): value(u32(ivalue)) {}

    constexpr statx_masks operator|(statx_mask flag) const {
        return statx_mask(value | u32(flag));
    }

    constexpr statx_masks operator&(statx_mask flag) const {
        return statx_mask(value & u32(flag));
    }

    constexpr statx_masks& operator|=(statx_mask flag) {
        value = u32(value | u32(flag));
        return *this;
    }

    constexpr bool test(statx_masks flags) const {
        return value & flags.value;
    }

    explicit constexpr operator bool() const {
        return value;
    }

    constexpr std::string to_string() const {
        constexpr core::array flags = {
            core::tuple{statx_mask::type, std::string_view("type")},
            core::tuple{statx_mask::mode, std::string_view("mode")},
            core::tuple{statx_mask::nlink, std::string_view("nlink")},
            core::tuple{statx_mask::uid, std::string_view("uid")},
            core::tuple{statx_mask::gid, std::string_view("gid")},
            core::tuple{statx_mask::atime, std::string_view("atime")},
            core::tuple{statx_mask::mtime, std::string_view("mtime")},
            core::tuple{statx_mask::ctime, std::string_view("ctime")},
            core::tuple{statx_mask::ino, std::string_view("ino")},
            core::tuple{statx_mask::size, std::string_view("size")},
            core::tuple{statx_mask::blocks, std::string_view("blocks")},
            core::tuple{statx_mask::basic_stats, std::string_view("basic_stats")},
            core::tuple{statx_mask::btime, std::string_view("btime")},
            core::tuple{statx_mask::all, std::string_view("all")},
            core::tuple{statx_mask::mnt_id, std::string_view("mnt_id")},
            core::tuple{statx_mask::dio_mem_align, std::string_view("dio_mem_align")},
        };
        std::string res;
        for (auto&& [f, s] : flags) {
            if (u32(value) & u32(f)) {
                res.append(s);
                res.append(" | ");
            }
        }
        if (res.size() > 2)
            res.resize(res.size() - 2);
        return res;
    }

    u32 value;
};


inline constexpr statx_masks operator|(statx_mask lhs, statx_mask rhs) {
    return statx_mask(u32(lhs) | u32(rhs));
}
/* [[[codegen end]]] */

/*
 * [[[codegen start]]]
 * Generated with:
 * ./build-deb/codegen/flags statx_attr statx_attrs u64 compressed=STATX_ATTR_COMPRESSED immutable=STATX_ATTR_IMMUTABLE
 * append=STATX_ATTR_APPEND nodump=STATX_ATTR_NODUMP encrypted=STATX_ATTR_ENCRYPTED verity=STATX_ATTR_VERITY
 * dax=STATX_ATTR_DAX mount_root=STATX_ATTR_MOUNT_ROOT
 */
enum class statx_attr : u64 {
    compressed = STATX_ATTR_COMPRESSED,
    immutable  = STATX_ATTR_IMMUTABLE,
    append     = STATX_ATTR_APPEND,
    nodump     = STATX_ATTR_NODUMP,
    encrypted  = STATX_ATTR_ENCRYPTED,
    verity     = STATX_ATTR_VERITY,
    dax        = STATX_ATTR_DAX,
    mount_root = STATX_ATTR_MOUNT_ROOT,
};

struct statx_attrs {
    constexpr statx_attrs() = default;
    constexpr statx_attrs(statx_attr ivalue): value(u64(ivalue)) {}

    constexpr statx_attrs operator|(statx_attr flag) const {
        return statx_attr(value | u64(flag));
    }

    constexpr statx_attrs operator&(statx_attr flag) const {
        return statx_attr(value & u64(flag));
    }

    constexpr statx_attrs& operator|=(statx_attr flag) {
        value = u64(value | u64(flag));
        return *this;
    }

    constexpr bool test(statx_attrs flags) const {
        return value & flags.value;
    }

    explicit constexpr operator bool() const {
        return value;
    }

    constexpr std::string to_string() const {
        constexpr core::array flags = {
            core::tuple{statx_attr::compressed, std::string_view("compressed")},
            core::tuple{statx_attr::immutable, std::string_view("immutable")},
            core::tuple{statx_attr::append, std::string_view("append")},
            core::tuple{statx_attr::nodump, std::string_view("nodump")},
            core::tuple{statx_attr::encrypted, std::string_view("encrypted")},
            core::tuple{statx_attr::verity, std::string_view("verity")},
            core::tuple{statx_attr::dax, std::string_view("dax")},
            core::tuple{statx_attr::mount_root, std::string_view("mount_root")},
        };
        std::string res;
        for (auto&& [f, s] : flags) {
            if (u64(value) & u64(f)) {
                res.append(s);
                res.append(" | ");
            }
        }
        if (res.size() > 2)
            res.resize(res.size() - 2);
        return res;
    }

    u64 value;
};

inline constexpr statx_attrs operator|(statx_attr lhs, statx_attr rhs) {
    return statx_attr(u64(lhs) | u64(rhs));
}
/* [[[codegen end]]] */


/*
 * [[[codegen start]]]
 * Generated with:
 * ./build-deb/codegen/flags statx_flag statx_flags uint empty_path=AT_EMPTY_PATH no_automount=AT_NO_AUTOMOUNT symlink_nofollow=AT_SYMLINK_NOFOLLOW sync_as_stat=AT_STATX_SYNC_AS_STAT force_sync=AT_STATX_FORCE_SYNC dont_sync=AT_STATX_DONT_SYNC
 */
enum class statx_flag : uint {
    empty_path       = AT_EMPTY_PATH,
    no_automount     = AT_NO_AUTOMOUNT,
    symlink_nofollow = AT_SYMLINK_NOFOLLOW,
    sync_as_stat     = AT_STATX_SYNC_AS_STAT,
    force_sync       = AT_STATX_FORCE_SYNC,
    dont_sync        = AT_STATX_DONT_SYNC,
};

struct statx_flags {
    constexpr statx_flags() = default;
    constexpr statx_flags(statx_flag ivalue): value(uint(ivalue)) {}

    constexpr statx_flags operator|(statx_flag flag) const {
        return statx_flag(value | uint(flag));
    }

    constexpr statx_flags operator&(statx_flag flag) const {
        return statx_flag(value & uint(flag));
    }

    constexpr statx_flags& operator|=(statx_flag flag) {
        value = uint(value | uint(flag));
        return *this;
    }

    constexpr bool test(statx_flags flags) const {
        return value & flags.value;
    }

    explicit constexpr operator bool() const {
        return value;
    }

    constexpr void unset(statx_flags flags) {
        value &= uint(~flags.value);
    }

    constexpr std::string to_string() const {
        constexpr core::array flags = {
            core::tuple{statx_flag::empty_path, std::string_view("empty_path")},
            core::tuple{statx_flag::no_automount, std::string_view("no_automount")},
            core::tuple{statx_flag::symlink_nofollow, std::string_view("symlink_nofollow")},
            core::tuple{statx_flag::sync_as_stat, std::string_view("sync_as_stat")},
            core::tuple{statx_flag::force_sync, std::string_view("force_sync")},
            core::tuple{statx_flag::dont_sync, std::string_view("dont_sync")},
        };
        std::string res;
        for (auto&& [f, s] : flags) {
            if (uint(value) & uint(f)) {
                res.append(s);
                res.append(" | ");
            }
        }
        if (res.size() > 2)
            res.resize(res.size() - 2);
        return res;
    }

    uint value;
};

inline constexpr statx_flags operator|(statx_flag lhs, statx_flag rhs) {
    return statx_flag(uint(lhs) | uint(rhs));
}
/* [[[codegen end]]] */

struct statx_timestamp {
    constexpr operator nanoseconds() const {
        return value();
    }

    constexpr nanoseconds value() const {
        return seconds{sec} + nanoseconds{nsec};
    }

    i64 sec;
    u32 nsec, __pad;
};

enum class file_type : u16 {
    socket  = S_IFSOCK,
    symlink = S_IFLNK,
    regular = S_IFREG,
    block   = S_IFBLK,
    dir     = S_IFDIR,
    chardev = S_IFCHR,
    fifo    = S_IFIFO,
};

struct statx_mode {
    auto type() const {
        return file_type{u16(_data & S_IFMT)};
    }

    auto perms() const {
        return file_perms::from_number(_data & u16(~u16(S_IFMT)));
    }

    u16 _data;
};

struct statx_info {
    statx_masks     mask;
    u32             blksize;
    statx_attrs     attributes;
    u32             nlink;
    u32             uid;
    u32             gid;
    statx_mode      mode;
    u16             __pad0[1];
    u64             ino;
    u64             size;
    u64             blocks;
    statx_attrs     attributes_mask;
    statx_timestamp atime;
    statx_timestamp btime;
    statx_timestamp ctime;
    statx_timestamp mtime;
    u32             rdev_major;
    u32             rdev_minor;
    u32             dev_major;
    u32             dev_minor;
    u64             __pad1[14];
};
} // namespace sys
