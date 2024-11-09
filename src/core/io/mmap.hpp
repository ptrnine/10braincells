#pragma once

#include <core/io/basic_types.hpp>
#include <core/io/file.hpp>
#include <core/meta/overload_resolution.hpp>
#include <core/traits/conditional.hpp>
#include <core/byteview.hpp>

#include <sys/ftruncate.hpp>
#include <sys/mmap.hpp>
#include <sys/statx.hpp>

#define fwd(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace core::io
{
namespace details
{
    template <typename... Args>
    constexpr auto deduce_mmap_fd() {
        return type<conditional<type_list<Args...>.contains(type<file&&>), file, fd_t>>;
    }
} // namespace details

template <typename Fd>
class mmap {
public:
    constexpr mmap() = default;

    constexpr ~mmap() {
        if (_data.not_default())
            sys::munmap(_data.get(), _mmap_sz);
    }

    mmap(mmap&&)                 = default;
    mmap& operator=(mmap&&)      = default;

    template <typename Arg>
        requires(!same_as<Arg, mmap>)
    constexpr mmap(Arg&& arg, auto&&... args) {
        bool size_init     = false;
        bool write_enabled = false;

        if constexpr (is_same<decltype(_fd), fd_t>)
            _fd = invalid_fd;

        fwd_as_tuple(fwd(arg), fwd(args)...)
            .foreach (overloaded{
                [&](file&& fd) {
                    _fd = fwd(fd);
                },
                [&](const fd_t& fd) {
                    if constexpr (is_same<decltype(_fd), fd_t>)
                        _fd = fd_t(fd);
                },
                [&](const integral auto& s) {
                    _mmap_sz  = s;
                    size_init = true;
                },
                [&](map_prots prot) {
                    if (prot.test(map_prots::write))
                        write_enabled = true;
                },
                overload_default<map_flags, sys::map_offset>,
            });

        if (_fd != invalid_fd) {
            if (!size_init) {
                if (auto stx = sys::statx(_fd, sys::statx_mask::size)) {
                    if (stx->size == 0 && write_enabled)
                        sys::ftruncate(_fd, _mmap_sz).throw_if_error();
                    else
                        _mmap_sz = stx->size;
                }
            }
            else if (write_enabled) {
                if (auto stx = sys::statx(_fd, sys::statx_mask::size)) {
                    if (stx->size < _mmap_sz)
                        sys::ftruncate(_fd, _mmap_sz).throw_if_error();
                }
            }
        }

        _data = fwd_as_tuple(fwd(arg), fwd(args)...)
                    .cat(tuple{fd_t(_fd), _mmap_sz})
                    .pass_to([](auto&&... args) {
                        return sys::mmap(fwd(args)...);
                    })
                    .get();
    }

    template <typename T = byte>
    constexpr auto data() const {
        return (T*)_data.get();
    }

    constexpr size_t size() const {
        return _mmap_sz;
    }

    fd_t fd() const {
        return _fd;
    }

    bool valid() const {
        return _data.not_default();
    }

    explicit operator bool() const {
        return valid();
    }

    void truncate(size_t new_size) {
        if (_fd != invalid_fd) {
            sys::ftruncate(_fd, off_t(new_size)).throw_if_error();
            remap(new_size);
        }
    }

    void remap(size_t new_sz, bool may_move = true) {
        _data = sys::mremap(_data.get(), _mmap_sz, new_sz, may_move ? sys::remap_flags::may_move : sys::remap_flags{})
                    .get();
        _mmap_sz = new_sz;
    }

    template <trivial T>
    auto&& from_byteview(this auto&& it, size_t start = 0) {
        return core::from_byteview<T>((char*)it._data.get() + start, std::min(sizeof(T), it._mmap_sz - start));
    }

private:
    Fd                               _fd;
    moveonly_trivial<void*, nullptr> _data    = nullptr;
    size_t                           _mmap_sz = 0;
};

template <typename... Args>
mmap(Args&&...) -> mmap<typename decltype(details::deduce_mmap_fd<Args&&...>())::type>;
} // namespace core::io

#undef fwd
