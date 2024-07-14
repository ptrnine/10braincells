#pragma once

#include <cstdint>

#include <core/traits/make_signed.hpp>

namespace sys
{
using size_t  = std::size_t;
using diff_t  = std::ptrdiff_t;
using ssize_t = core::make_signed<size_t>;
using uptr    = uintptr_t;

using u8  = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using i8  = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using uint = unsigned int;

using char8  = char8_t;
using char16 = char16_t;
using char32 = char32_t;

using f32 = float;
using f64 = double;

enum class byte : u8 {};

enum class fd_t : int {};
} // namespace sys
