#pragma once

#include <core/array.hpp>
#include <core/basic_types.hpp>
#include <core/bits.hpp>

/*
 * xxHash (XXH32 / XXH64) - non-cryptographic 32/64-bit hash.
 *
 * Bit-exact port of the reference implementation from https://github.com/Cyan4973/xxHash
 * (xxhash.h, BSD 2-clause). Input bytes are read as little-endian values; on this
 * project's target (x86_64) the results match the upstream reference exactly,
 * which is what the known-answer test vectors in tests/core/xxhash.cpp verify.
 *
 * API:
 *   u64 h  = xxh64(data, size);              // one-shot, fastest path
 *   u32 h32 = xxh32(data, size, seed);       // one-shot 32-bit
 *
 *   xxhash64 st{seed};                       // streaming
 *   st.update(chunk, chunk_size);
 *   st.update(chunk2, chunk2_size);
 *   u64 h = st.digest();                     // digest is non-destructive
 *
 *   st.reset(new_seed);                      // reuse the state
 */

namespace core {

namespace xxh_detail {

/* Little-endian unaligned loads (safe for any alignment). */
inline constexpr u64 read_le64(const byte* p) noexcept {
    u64 v;
    __builtin_memcpy(&v, p, sizeof(v));
    return v;
}
inline constexpr u32 read_le32(const byte* p) noexcept {
    u32 v;
    __builtin_memcpy(&v, p, sizeof(v));
    return v;
}

/* ------------ XXH64 primitives ------------ */

inline constexpr u64 xxh64_round(u64 acc, u64 x) noexcept {
    acc += x * 0xC2B2AE3D27D4EB4FULL;
    return rotl(acc, 31) * 0x9E3779B185EBCA87ULL;
}

inline constexpr u64 xxh64_avalanche(u64 h) noexcept {
    h ^= h >> 33;
    h *= 0xC2B2AE3D27D4EB4FULL;
    h ^= h >> 29;
    h *= 0x165667B19E3779F9ULL;
    h ^= h >> 32;
    return h;
}

inline constexpr u64 xxh64_merge_round(u64 acc, u64 val) noexcept {
    acc ^= xxh64_round(0, val);
    return acc * 0x9E3779B185EBCA87ULL + 0x85EBCA77C2B2AE63ULL;
}

inline constexpr u64 xxh64_merge_accs(const array<u64, 4>& acc) noexcept {
    u64 h = rotl(acc[0], 1) + rotl(acc[1], 7) + rotl(acc[2], 12) + rotl(acc[3], 18);
    for (size_t i = 0; i < 4; ++i)
        h = xxh64_merge_round(h, acc[i]);
    return h;
}

/* Consume len bytes (len >= 32), returns pointer to the last < 32 bytes. */
inline constexpr const byte* xxh64_consume_long(array<u64, 4>& acc, const byte* p, u64 len) noexcept {
    const byte* const end   = p + len;
    const byte* const limit = end - 31;
    do {
        acc[0] = xxh64_round(acc[0], read_le64(p));
        p += 8;
        acc[1] = xxh64_round(acc[1], read_le64(p));
        p += 8;
        acc[2] = xxh64_round(acc[2], read_le64(p));
        p += 8;
        acc[3] = xxh64_round(acc[3], read_le64(p));
        p += 8;
    } while (p < limit);
    return p;
}

/* Digest the final < 32 bytes into h. */
inline constexpr u64 xxh64_finalize(u64 h, const byte* p, u64 len) noexcept {
    len &= 31;
    while (len >= 8) {
        h ^= xxh64_round(0, read_le64(p));
        p += 8;
        h = rotl(h, 27) * 0x9E3779B185EBCA87ULL + 0x85EBCA77C2B2AE63ULL;
        len -= 8;
    }
    if (len >= 4) {
        h ^= (u64)read_le32(p) * 0x9E3779B185EBCA87ULL;
        p += 4;
        h = rotl(h, 23) * 0xC2B2AE3D27D4EB4FULL + 0x165667B19E3779F9ULL;
        len -= 4;
    }
    while (len > 0) {
        h ^= (u64)(*p++) * 0x27D4EB2F165667C5ULL;
        h = rotl(h, 11) * 0x9E3779B185EBCA87ULL;
        --len;
    }
    return xxh64_avalanche(h);
}

/* ------------ XXH32 primitives ------------ */

inline constexpr u32 xxh32_round(u32 acc, u32 x) noexcept {
    acc += x * 2246822519u;
    return rotl(acc, 13) * 2654435761u;
}

inline constexpr u32 xxh32_avalanche(u32 h) noexcept {
    h ^= h >> 15;
    h *= 2246822519u;
    h ^= h >> 13;
    h *= 3266489917u;
    h ^= h >> 16;
    return h;
}

inline constexpr u32 xxh32_merge_accs(const array<u32, 4>& acc) noexcept {
    return rotl(acc[0], 1) + rotl(acc[1], 7) + rotl(acc[2], 12) + rotl(acc[3], 18);
}

/* Consume len bytes (len >= 16), returns pointer to the last < 16 bytes. */
inline constexpr const byte* xxh32_consume_long(array<u32, 4>& acc, const byte* p, u64 len) noexcept {
    const byte* const end   = p + len;
    const byte* const limit = end - 15;
    do {
        acc[0] = xxh32_round(acc[0], read_le32(p));
        p += 4;
        acc[1] = xxh32_round(acc[1], read_le32(p));
        p += 4;
        acc[2] = xxh32_round(acc[2], read_le32(p));
        p += 4;
        acc[3] = xxh32_round(acc[3], read_le32(p));
        p += 4;
    } while (p < limit);
    return p;
}

/* Digest the final < 16 bytes into h. */
inline constexpr u32 xxh32_finalize(u32 h, const byte* p, u64 len) noexcept {
    len &= 15;
    while (len >= 4) {
        h += read_le32(p) * 3266489917u;
        p += 4;
        h = rotl(h, 17) * 668265263u;
        len -= 4;
    }
    while (len > 0) {
        h += (u32)(*p++) * 374761393u;
        h = rotl(h, 11) * 2654435761u;
        --len;
    }
    return xxh32_avalanche(h);
}

} // namespace xxh_detail

/* ============================================================================
 * xxhash32 - streaming 32-bit xxHash
 * ==========================================================================*/
class xxhash32 {
public:
    static constexpr u32 prime1 = 2654435761u;
    static constexpr u32 prime2 = 2246822519u;
    static constexpr u32 prime3 = 3266489917u;
    static constexpr u32 prime4 = 668265263u;
    static constexpr u32 prime5 = 374761393u;

    explicit constexpr xxhash32(u32 seed = 0) noexcept
        : _acc{seed + prime1 + prime2, seed + prime2, seed, seed - prime1} {}

    /* One-shot hash of a contiguous buffer. */
    static u32 hash(const void* data, u64 size, u32 seed = 0) noexcept {
        u32           h;
        const byte*   p = (const byte*)data;
        array<u32, 4> acc{seed + prime1 + prime2, seed + prime2, seed, seed - prime1};

        if (size >= 16) {
            p = xxh_detail::xxh32_consume_long(acc, p, size);
            h = xxh_detail::xxh32_merge_accs(acc);
        }
        else
            h = seed + prime5;

        h += (u32)size;
        return xxh_detail::xxh32_finalize(h, p, size & 15);
    }

    /* Feed another chunk into the stream. Null/zero-size is a no-op. */
    void update(const void* data, u64 size) noexcept {
        if (size == 0)
            return;

        _total_size += size;

        const byte* const p   = (const byte*)data;
        const byte* const end = p + size;

        if (size < 16 - _buff_size) { // buffer it
            __builtin_memcpy(_buff.data() + _buff_size, p, size);
            _buff_size = (u32)(_buff_size + size);
            return;
        }

        const byte* x = p;
        if (_buff_size) { // complete and flush the pending buffer
            __builtin_memcpy(_buff.data() + _buff_size, p, 16 - _buff_size);
            x += 16 - _buff_size;
            xxh_detail::xxh32_consume_long(_acc, _buff.data(), 16);
            _buff_size = 0;
        }

        const byte* rem = x;
        if ((u64)(end - x) >= 16)
            rem = xxh_detail::xxh32_consume_long(_acc, x, (u64)(end - x));

        if (rem < end) {
            __builtin_memcpy(_buff.data(), rem, (u64)(end - rem));
            _buff_size = (u32)(end - rem);
        }
    }

    /* Final hash. Non-destructive: may be called repeatedly, before or after updates. */
    u32 digest() const noexcept {
        u32 h = _total_size >= 16 ? xxh_detail::xxh32_merge_accs(_acc) : _acc[2] + prime5;
        h += (u32)_total_size;
        return xxh_detail::xxh32_finalize(h, _buff.data(), _buff_size);
    }

    /* Reset the state with a new seed (or the same one). */
    void reset(u32 seed = 0) noexcept {
        _acc       = {seed + prime1 + prime2, seed + prime2, seed, seed - prime1};
        _buff_size = 0;
        _total_size = 0;
    }

    u64 length() const noexcept {
        return _total_size;
    }

private:
    array<u32, 4> _acc;
    array<byte, 16> _buff;
    u32 _buff_size   = 0;
    u64 _total_size  = 0;
};

/* ============================================================================
 * xxhash64 - streaming 64-bit xxHash
 * ==========================================================================*/
class xxhash64 {
public:
    static constexpr u64 prime1 = 0x9E3779B185EBCA87ULL;
    static constexpr u64 prime2 = 0xC2B2AE3D27D4EB4FULL;
    static constexpr u64 prime3 = 0x165667B19E3779F9ULL;
    static constexpr u64 prime4 = 0x85EBCA77C2B2AE63ULL;
    static constexpr u64 prime5 = 0x27D4EB2F165667C5ULL;

    explicit constexpr xxhash64(u64 seed = 0) noexcept
        : _acc{seed + prime1 + prime2, seed + prime2, seed, seed - prime1} {}

    /* One-shot hash of a contiguous buffer. */
    static u64 hash(const void* data, u64 size, u64 seed = 0) noexcept {
        u64           h;
        const byte*   p = (const byte*)data;
        array<u64, 4> acc{seed + prime1 + prime2, seed + prime2, seed, seed - prime1};

        if (size >= 32) {
            p = xxh_detail::xxh64_consume_long(acc, p, size);
            h = xxh_detail::xxh64_merge_accs(acc);
        }
        else
            h = seed + prime5;

        h += size;
        return xxh_detail::xxh64_finalize(h, p, size & 31);
    }

    /* Feed another chunk into the stream. Null/zero-size is a no-op. */
    void update(const void* data, u64 size) noexcept {
        if (size == 0)
            return;

        _total_size += size;

        const byte* const p   = (const byte*)data;
        const byte* const end = p + size;

        if (size < 32 - _buff_size) { // buffer it
            __builtin_memcpy(_buff.data() + _buff_size, p, size);
            _buff_size = (u32)(_buff_size + size);
            return;
        }

        const byte* x = p;
        if (_buff_size) { // complete and flush the pending buffer
            __builtin_memcpy(_buff.data() + _buff_size, p, 32 - _buff_size);
            x += 32 - _buff_size;
            xxh_detail::xxh64_consume_long(_acc, _buff.data(), 32);
            _buff_size = 0;
        }

        const byte* rem = x;
        if ((u64)(end - x) >= 32)
            rem = xxh_detail::xxh64_consume_long(_acc, x, (u64)(end - x));

        if (rem < end) {
            __builtin_memcpy(_buff.data(), rem, (u64)(end - rem));
            _buff_size = (u32)(end - rem);
        }
    }

    /* Final hash. Non-destructive: may be called repeatedly, before or after updates. */
    u64 digest() const noexcept {
        u64 h = _total_size >= 32 ? xxh_detail::xxh64_merge_accs(_acc) : _acc[2] + prime5;
        h += _total_size;
        return xxh_detail::xxh64_finalize(h, _buff.data(), _buff_size);
    }

    /* Reset the state with a new seed (or the same one). */
    void reset(u64 seed = 0) noexcept {
        _acc = {seed + prime1 + prime2, seed + prime2, seed, seed - prime1};
        _buff_size  = 0;
        _total_size = 0;
    }

    u64 length() const noexcept {
        return _total_size;
    }

private:
    array<u64, 4> _acc;
    array<byte, 32> _buff;
    u32 _buff_size   = 0;
    u64 _total_size  = 0;
};

/* ============================================================================
 * One-shot convenience functions
 * ==========================================================================*/
constexpr u32 xxh32(const void* data, u64 size, u32 seed = 0) noexcept {
    return xxhash32::hash(data, size, seed);
}
constexpr u64 xxh64(const void* data, u64 size, u64 seed = 0) noexcept {
    return xxhash64::hash(data, size, seed);
}

} // namespace core
