/* Benchmark: core::xxhash (src/core/xxhash.hpp) vs upstream xxhash (../xxhash)
 *
 * Each iteration alternates between two halves of a 2*n byte buffer so the
 * compiler cannot hoist the loads out of the timing loop (identical input
 * every iteration gets its loads loop-invariant-hoisted and the measured
 * time becomes meaningless). */
#include <core/xxhash.hpp>
#include <core/compact_hashes.hpp>
#define XXH_STATIC_LINKING_ONLY
#include <xxhash.h>

#include <chrono>
#include <cstdio>
#include <vector>

using namespace core;

// Never mutated: a volatile read defeats loop-invariant folding for the n==0
// (pure constant) benchmark loops.
static volatile u64 g_volatile_seed = 0;
static volatile u32 g_volatile_seed32 = 0;

static double now_ns() {
    using clock = std::chrono::steady_clock;
    return std::chrono::duration<double, std::nano>(clock::now().time_since_epoch()).count();
}

static std::vector<u8> make_data(size_t n) {
    std::vector<u8> v(n);
    for (size_t i = 0; i < n; ++i)
        v[i] = (u8)(i * 2654435761u + (i >> 13));
    return v;
}

static inline int clampi(long long x, long long lo, long long hi) {
    return x < lo ? (int)lo : (x > hi ? (int)hi : (int)x);
}

int main() {
    u64 sink = 0;
    const size_t sizes[] = {0, 1, 7, 15, 31, 32, 33, 63, 64, 127, 255, 1024, 4096, 65536, 1024 * 1024};

    printf("== one-shot ==\n");
    printf("%-8s | %26s | %26s | %26s | %10s\n", "size", "core::xxh64", "XXH64 (upstream)", "core::fnv1a64", "xxh64/upstr");
    for (size_t n : sizes) {
        auto data  = make_data(2 * n + 8);
        int iters  = clampi((long long)(2000000000ULL / (n + 1)), 20000, 50000000);
        u64 a0     = 0, b0 = 0, c0 = 0;
        for (int i = 0; i < 100; ++i) { // warmup
            a0 += core::xxh64(data.data() + ((size_t)i & 1) * n, n);
            b0 += XXH64(data.data() + ((size_t)i & 1) * n, n, 0);
            c0 += core::fnv1a64(data.data() + ((size_t)i & 1) * n, n);
        }
        sink += a0 + b0 + c0;
        double t0 = now_ns();
        u64 a = 0;
        for (int i = 0; i < iters; ++i)
            a += core::xxh64(data.data() + ((size_t)i & 1) * n, n, n == 0 ? g_volatile_seed : 0);
        double t1 = now_ns();
        u64 b = 0;
        for (int i = 0; i < iters; ++i)
            b += XXH64(data.data() + ((size_t)i & 1) * n, n, n == 0 ? g_volatile_seed : 0);
        double t2 = now_ns();
        u64 c = 0;
        volatile size_t sz = n; // volatile keeps the n==0 (constant) loop live
        for (int i = 0; i < iters; ++i)
            c += core::fnv1a64(data.data() + ((size_t)i & 1) * n, sz);
        double t3 = now_ns();
        sink += a + b + c;
        double na = (t1 - t0) / iters;
        double nb = (t2 - t1) / iters;
        double nc = (t3 - t2) / iters;
        printf("%-8zu | %9.2f ns  %9.1f GB/s | %9.2f ns  %9.1f GB/s | %9.2f ns  %9.1f GB/s | %10.2fx\n",
               n, na, (double)n / na * 1e3, nb, (double)n / nb * 1e3, nc, (double)n / nc * 1e3, na / nb);
    }

    printf("\n== one-shot 32-bit ==\n");
    printf("%-8s | %26s | %26s | %10s\n", "size", "core::xxh32", "XXH32 (upstream)", "core/upstr");
    for (size_t n : sizes) {
        auto data  = make_data(2 * n + 8);
        int iters  = clampi((long long)(2000000000ULL / (n + 1)), 20000, 50000000);
        u64 a0     = 0, b0 = 0;
        for (int i = 0; i < 100; ++i) { // warmup
            a0 += core::xxh32(data.data() + ((size_t)i & 1) * n, n, n == 0 ? g_volatile_seed32 : 0);
            b0 += XXH32(data.data() + ((size_t)i & 1) * n, n, n == 0 ? g_volatile_seed32 : 0);
        }
        sink += a0 + b0;
        double t0 = now_ns();
        u64 a = 0;
        for (int i = 0; i < iters; ++i)
            a += core::xxh32(data.data() + ((size_t)i & 1) * n, n, n == 0 ? g_volatile_seed32 : 0);
        double t1 = now_ns();
        u64 b = 0;
        for (int i = 0; i < iters; ++i)
            b += XXH32(data.data() + ((size_t)i & 1) * n, n, n == 0 ? g_volatile_seed32 : 0);
        double t2 = now_ns();
        sink += a + b;
        double na = (t1 - t0) / iters;
        double nb = (t2 - t1) / iters;
        printf("%-8zu | %9.2f ns  %9.1f GB/s | %9.2f ns  %9.1f GB/s | %10.2fx\n",
               n, na, (double)n / na * 1e3, nb, (double)n / nb * 1e3, na / nb);
    }

    printf("\n== streaming (64 KiB chunks, update + digest) ==\n");
    printf("%-8s | %26s | %26s | %10s\n", "size", "core::xxhash64::update", "XXH64_update", "core/upstr");
    for (size_t n : sizes) {
        auto data = make_data(2 * n + 8);
        int iters = clampi((long long)(2000000000ULL / (n + 1)), 20000, 5000000);
        u64 a = 0;
        double t0 = now_ns();
        for (int i = 0; i < iters; ++i) {
            const u8* p       = data.data() + ((size_t)i & 1) * n;
            xxhash64 st{n == 0 ? g_volatile_seed : 0}; // volatile seed keeps the n==0 loop live
            for (size_t off = 0; off < n; off += 65536)
                st.update(p + off, (n - off) < 65536 ? (n - off) : 65536);
            a += st.digest();
        }
        double t1 = now_ns();
        u64 b = 0;
        for (int i = 0; i < iters; ++i) {
            const u8* p = data.data() + ((size_t)i & 1) * n;
            XXH64_state_t st;
            XXH64_reset(&st, n == 0 ? g_volatile_seed : 0);
            for (size_t off = 0; off < n; off += 65536)
                XXH64_update(&st, p + off, (n - off) < 65536 ? (n - off) : 65536);
            b += XXH64_digest(&st);
        }
        double t2 = now_ns();
        sink += a + b;
        double na = (t1 - t0) / iters;
        double nb = (t2 - t1) / iters;
        printf("%-8zu | %9.2f ns  %9.1f GB/s | %9.2f ns  %9.1f GB/s | %10.2fx\n",
               n, na, (double)n / na * 1e3, nb, (double)n / nb * 1e3, na / nb);
    }
    printf("sink=%llx\n", (unsigned long long)sink);
    return 0;
}
