#pragma once

#include <core/array.hpp>
#include <core/basic_types.hpp>
#include <core/bits.hpp>

namespace core {
/* TODO: implement this */
class xxhash64 {
public:
    static constexpr u64 prime1 = 0x9E3779B185EBCA87;
    static constexpr u64 prime2 = 0xC2B2AE3D27D4EB4F;
    static constexpr u64 prime3 = 0x165667B19E3779F9;
    static constexpr u64 prime4 = 0x85EBCA77C2B2AE63;
    static constexpr u64 prime5 = 0x27D4EB2F165667C5;

    xxhash64(u64 seed): _state{seed + prime1 + prime2, seed + prime2, seed, seed - prime1}, _buff_size(0), _total_size(0) {}

    bool add(const void* in, u64 size) {
        if (!in || size == 0)
            return false;

        _total_size += size;
        if (_buff_size + size < _buff.size()) {
            __builtin_memcpy(_buff.data() + _buff_size, in, size);
            return true;
        }

        auto remains = _buff.size() - _buff_size;
        __builtin_memcpy(_buff.data() + _buff_size, in, remains);
        calc_block(_buff.data());

        _buff_size = 0;
        _total_size += remains;
        in = (const byte*)in + remains;

        if (_buff_size > 0) {

        }

        auto p = (const byte*)in;
    }

private:
    static constexpr u64 calc_single(u64 state, u64 in) {
        return rotl(state + in * prime2, 31) * prime1;
    }

    constexpr void calc_block(const void* input) {
        auto in = (const u64*)input;
        for (size_t i = 0; i < _state.size(); ++i)
            _state[i] = calc_single(_state[i], in[i]);
    }

    array<u64, 4>   _state;
    array<byte, 32> _buff;
    u64             _buff_size;
    u64             _total_size;
};
}
