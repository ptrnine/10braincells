#pragma once

#include "../basic_types.hpp"

namespace core {
template <typename T, T...>
struct int_seq {};

template <size_t... Idxs>
using idx_seq = int_seq<size_t, Idxs...>;

#if __has_builtin(__make_integer_seq)
template <typename T, size_t size>
using make_int_seq = __make_integer_seq<int_seq, T, size>;

template <size_t size>
using make_idx_seq = __make_integer_seq<int_seq, size_t, size>;
#else
template <typename T, size_t size>
using make_int_seq = int_seq<T, __integer_pack(size)...>;

template <size_t size>
using make_idx_seq = int_seq<size_t, __integer_pack(size)...>;
#endif
} // namespace core
