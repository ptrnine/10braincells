#pragma once

#include "basic_types.hpp"

namespace core::bin {
template <typename T>
struct resource {
    const T* begin() const noexcept {
        return pstart;
    }

    const T* end() const noexcept {
        return pend;
    }

    size_t size() const noexcept {
        return sz;
    }

    const T* pstart;
    const T* pend;
    size_t   sz;
};
} // namespace core::bin

#define _INCBIN_STR2(x) #x
#define _INCBIN_STR(x) _INCBIN_STR2(x)

#define incbin_type_align(type, name, file, align)                                \
extern "C" const __attribute__((aligned(align))) type _incb##name##_data[];       \
extern "C" const __attribute__((aligned(align))) char _incb##name##_end;          \
extern "C" unsigned int _incb##name##_size;                                       \
namespace core::bin {                                                             \
    static const resource<type> name{                                             \
        _incb##name##_data,                                                       \
        reinterpret_cast<const type*>(&_incb##name##_end),                        \
        _incb##name##_size                                                        \
    };                                                                            \
}                                                                                 \
__asm__(".section .rodata\n"                                                      \
        ".global _incb" _INCBIN_STR(name) "_data\n"                               \
        ".type _incb" _INCBIN_STR(name) "_data, @object\n"                        \
        ".balign " _INCBIN_STR(align) "\n"                                        \
        ""                                                                        \
        "_incb" _INCBIN_STR(name) "_data:\n"                                      \
        ".incbin \"" file "\"\n"                                                  \
        ".global _incb" _INCBIN_STR(name) "_end\n"                                \
        ".type _incb" _INCBIN_STR(name) "_end, @object\n"                         \
        ".balign " _INCBIN_STR(align) "\n"                                        \
        ""                                                                        \
        "_incb" _INCBIN_STR(name) "_end:\n"                                       \
        ".int 0\n"                                                                \
        ".global _incb" _INCBIN_STR(name) "_size\n"                               \
        ".type _incb" _INCBIN_STR(name) "_size, @object\n"                        \
        ".balign " _INCBIN_STR(align) "\n"                                        \
        ""                                                                        \
        "_incb" _INCBIN_STR(name) "_size:\n"                                      \
        ".int _incb" _INCBIN_STR(name) "_end - _incb" _INCBIN_STR(name) "_data\n" \
        ".balign " _INCBIN_STR(align) "\n"                                        \
        ".text\n")                                                                \

#define incbin_type(type, name, file) incbin_type_align(type, name, file, 32)
#define incbin_align(name, file, align) incbin_type_align(core::byte, name, file, align)
#define incbin(name, file) incbin_align(name, file, 32)
