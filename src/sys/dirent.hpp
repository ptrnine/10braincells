#pragma once

#include <core/traits/is_array.hpp>
#include <string_view>
#include <string>
#include <sys/basic_types.hpp>

namespace sys {
enum class file_type_dt : u8 {
    unknown = 0,
    fifo    = 1,
    chardev = 2,
    dir     = 4,
    block   = 6,
    regular = 8,
    symlink = 10,
    socket  = 12,
};

struct dirent {
    ino_t        ino;
    off_t        offset;
    u16          reclen;
    file_type_dt type;
    char         name[];
};

struct dir_entry_str {
    ino_t        inode;
    file_type_dt type;
    std::string  name;
};

struct dir_entry {
    dir_entry_str to_dir_entry_str() {
        return {
            .inode = inode,
            .type  = type,
            .name  = std::string(name),
        };
    }

    ino_t            inode;
    file_type_dt     type;
    std::string_view name;
};

static inline constexpr size_t dirent_default_buffer_size = 4096;

template <typename BuffT = u8[dirent_default_buffer_size]>
class dirent_result {
public:
    dirent_result() = default;

    template <typename T = BuffT>
        requires(!core::is_array<BuffT>)
    dirent_result(auto* buff, size_t capacity): _data(buff), _capacity(capacity) {}

    dir_entry get() const {
        auto de = (dirent*)(_data + _pos);
        return dir_entry{
            .inode = de->ino,
            .type  = de->type,
            .name  = de->name,
        };
    }

    dirent* data() {
        return (dirent*)(_data + _pos);
    }

    void set_size(size_t size) {
        _size = size;
    }

    size_t capacity() {
        return _capacity;
    }

    void next() {
        _pos += ((dirent*)(_data + _pos))->reclen;
    }

    bool is_end() const {
        return _pos >= _size;
    }

    auto* buffer() {
        return _data;
    }

private:
    BuffT  _data;
    size_t _capacity = sizeof(BuffT);
    size_t _pos = 0;
    size_t _size = 0;
};
} // namespace sys
