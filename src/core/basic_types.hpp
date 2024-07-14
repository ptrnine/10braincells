#pragma once

#include <sys/basic_types.hpp>

namespace core
{
using sys::diff_t;
using sys::size_t;
using sys::ssize_t;
using sys::uptr;

using sys::u16;
using sys::u32;
using sys::u64;
using sys::u8;

using sys::i16;
using sys::i32;
using sys::i64;
using sys::i8;

using sys::uint;

using sys::char16;
using sys::char32;
using sys::char8;

using sys::f32;
using sys::f64;

using sys::byte;

struct none_t {};
static inline constexpr none_t none{};
} // namespace core
