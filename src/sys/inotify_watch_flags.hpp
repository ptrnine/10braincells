/* Command: ./build/codegen/cg_flags name=inotify_watch_flag name_combined=inotify_watch_flags type=uint namespace_name=sys access=0x00000001 modify=0x00000002
 * attrib=0x00000004 close_write=0x00000008 close_nowrite=0x00000010 close=close_write | close_nowrite open=0x00000020 moved_from=0x00000040 moved_to=0x00000080
 * move=moved_from | moved_to create=0x00000100 delete_=0x00000200 delete_self=0x00000400 move_self=0x00000800 unmount=0x00002000 q_overflow=0x00004000
 * ignored=0x00008000 close=close_write | close_nowrite move=moved_from | moved_to onlydir=0x01000000 dont_follow=0x02000000 excl_unlink=0x04000000
 * mask_create=0x10000000 mask_add=0x20000000 isdir=0x40000000 oneshot=0x80000000 all_events=access | modify | attrib | close_write | close_nowrite | open |
 * moved_from | moved_to | create | delete_ | delete_self | move_self */
#pragma once

#include <string>

#include <core/array.hpp>
#include <core/tuple.hpp>

namespace sys {
enum class inotify_watch_flag : uint {
    access        = 0x00000001,
    modify        = 0x00000002,
    attrib        = 0x00000004,
    close_write   = 0x00000008,
    close_nowrite = 0x00000010,
    close         = close_write | close_nowrite,
    open          = 0x00000020,
    moved_from    = 0x00000040,
    moved_to      = 0x00000080,
    move          = moved_from | moved_to,
    create        = 0x00000100,
    delete_       = 0x00000200,
    delete_self   = 0x00000400,
    move_self     = 0x00000800,
    unmount       = 0x00002000,
    q_overflow    = 0x00004000,
    ignored       = 0x00008000,
    onlydir       = 0x01000000,
    dont_follow   = 0x02000000,
    excl_unlink   = 0x04000000,
    mask_create   = 0x10000000,
    mask_add      = 0x20000000,
    isdir         = 0x40000000,
    oneshot       = 0x80000000,
    all_events    = access | modify | attrib | close_write | close_nowrite | open | moved_from | moved_to | create | delete_ | delete_self | move_self,
};

struct inotify_watch_flags {
    using enum inotify_watch_flag;

    constexpr inotify_watch_flags() = default;
    constexpr inotify_watch_flags(inotify_watch_flag ivalue): value(int(ivalue)) {}

    constexpr inotify_watch_flags operator|(inotify_watch_flags flag) const {
        return inotify_watch_flag(int(value | flag.value));
    }

    constexpr inotify_watch_flags operator&(inotify_watch_flags flag) const {
        return inotify_watch_flag(int(value & flag.value));
    }

    constexpr inotify_watch_flags& operator|=(inotify_watch_flags flag) {
        value = int(value | flag.value);
        return *this;
    }

    constexpr bool test(inotify_watch_flags flags) const {
        return value & flags.value;
    }

    explicit constexpr operator bool() const {
        return value;
    }

    constexpr void unset(inotify_watch_flags flags) {
        value &= int(~flags.value);
    }

    constexpr std::string to_string() const {
        constexpr core::array flags = {
            core::tuple{inotify_watch_flag::access, std::string_view("access")},
            core::tuple{inotify_watch_flag::modify, std::string_view("modify")},
            core::tuple{inotify_watch_flag::attrib, std::string_view("attrib")},
            core::tuple{inotify_watch_flag::close_write, std::string_view("close_write")},
            core::tuple{inotify_watch_flag::close_nowrite, std::string_view("close_nowrite")},
            core::tuple{inotify_watch_flag::close, std::string_view("close")},
            core::tuple{inotify_watch_flag::open, std::string_view("open")},
            core::tuple{inotify_watch_flag::moved_from, std::string_view("moved_from")},
            core::tuple{inotify_watch_flag::moved_to, std::string_view("moved_to")},
            core::tuple{inotify_watch_flag::move, std::string_view("move")},
            core::tuple{inotify_watch_flag::create, std::string_view("create")},
            core::tuple{inotify_watch_flag::delete_, std::string_view("delete_")},
            core::tuple{inotify_watch_flag::delete_self, std::string_view("delete_self")},
            core::tuple{inotify_watch_flag::move_self, std::string_view("move_self")},
            core::tuple{inotify_watch_flag::unmount, std::string_view("unmount")},
            core::tuple{inotify_watch_flag::q_overflow, std::string_view("q_overflow")},
            core::tuple{inotify_watch_flag::ignored, std::string_view("ignored")},
            core::tuple{inotify_watch_flag::close, std::string_view("close")},
            core::tuple{inotify_watch_flag::move, std::string_view("move")},
            core::tuple{inotify_watch_flag::onlydir, std::string_view("onlydir")},
            core::tuple{inotify_watch_flag::dont_follow, std::string_view("dont_follow")},
            core::tuple{inotify_watch_flag::excl_unlink, std::string_view("excl_unlink")},
            core::tuple{inotify_watch_flag::mask_create, std::string_view("mask_create")},
            core::tuple{inotify_watch_flag::mask_add, std::string_view("mask_add")},
            core::tuple{inotify_watch_flag::isdir, std::string_view("isdir")},
            core::tuple{inotify_watch_flag::oneshot, std::string_view("oneshot")},
            core::tuple{inotify_watch_flag::all_events, std::string_view("all_events")},
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

inline constexpr inotify_watch_flags operator|(inotify_watch_flag lhs, inotify_watch_flag rhs) {
    return inotify_watch_flag(int(lhs) | int(rhs));
}
} // namespace sys
