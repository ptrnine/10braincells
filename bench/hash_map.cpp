/* Benchmark: core::hash_map (src/core/robin_map.hpp) vs std::unordered_map.
 *
 * Workloads:
 *   fill  - clear() + insert N key/value pairs (steady state, includes the
 *           first allocation; fill cost includes the clear of the previous pass)
 *   find  - N successful lookups, in shuffled key order (hot + cold buckets)
 *   miss  - N failed lookups of keys that are not in the map
 *
 * Keys are pre-generated; the lookup loops iterate a shuffled index
 * permutation so every iteration touches many different buckets, and the
 * accumulated values are sunk into a volatile global so the compiler cannot
 * delete the workload (a local map with no observable effects would be
 * dead-code-eliminated).
 */
#include <core/robin_map.hpp>

#include <chrono>
#include <cstdio>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

using namespace core;

static volatile u64 g_sink = 0;

static double now_ns() {
    using clock = std::chrono::steady_clock;
    return std::chrono::duration<double, std::nano>(clock::now().time_since_epoch()).count();
}

static inline int clampi(long long x, long long lo, long long hi) {
    return x < lo ? (int)lo : (x > hi ? (int)hi : (int)x);
}

static std::vector<size_t> make_order(size_t n) {
    std::vector<size_t> order(n);
    for (size_t i = 0; i < n; ++i)
        order[i] = i;
    std::mt19937_64 gen(0xC0FFEE42ull);
    std::shuffle(order.begin(), order.end(), gen);
    return order;
}

static std::vector<u64> make_u64_keys(size_t n, size_t offset) {
    std::vector<u64> keys(n);
    for (size_t i = 0; i < n; ++i)
        keys[i] = (u64(i) * 2654435761ull) + (u64)offset;
    return keys;
}

static std::vector<std::string> make_str_keys(size_t n, size_t offset) {
    std::vector<std::string> keys(n);
    for (size_t i = 0; i < n; ++i) {
        char buf[24];
        int len = std::snprintf(buf, sizeof(buf), "key_%zu", i + offset);
        keys[i].assign(buf, (size_t)len);
    }
    return keys;
}

/* 96-byte keys: long-enough strings to blow past any short-string fast path. */
static std::vector<std::string> make_long_str_keys(size_t n, size_t offset) {
    std::vector<std::string> keys(n);
    for (size_t i = 0; i < n; ++i) {
        char buf[96];
        int len = std::snprintf(buf, sizeof(buf), "key_%zu", i + offset);
        for (int c = len; c < 96; ++c)
            buf[c] = (char)('a' + ((i * 31u + (size_t)c) % 26u));
        keys[i].assign(buf, 96);
    }
    return keys;
}

static std::vector<u64> make_vals(size_t n) {
    std::vector<u64> vals(n);
    for (size_t i = 0; i < n; ++i)
        vals[i] = (u64(i) * 0x9E3779B97F4A7C15ull) + 1;
    return vals;
}

/* fill: clear + insert all N pairs; returns ns per insertion. */
template <typename M, typename K, typename V>
static double bench_fill(const std::vector<K>& keys, const std::vector<V>& vals, int passes) {
    M m;
    // Warmup: first pass (includes the initial allocation).
    for (size_t i = 0; i < keys.size(); ++i)
        m[keys[i]] = vals[i];
    m.clear();
    volatile size_t sz = m.size();
    (void)sz; // observe the map so the loops below are not DCE'd

    double t0 = now_ns();
    for (int p = 0; p < passes; ++p) {
        m.clear();
        for (size_t i = 0; i < keys.size(); ++i)
            m[keys[i]] = vals[i];
        volatile size_t s2 = m.size();
        (void)s2;
    }
    double t1 = now_ns();
    return (t1 - t0) / ((double)passes * (double)keys.size());
}

/* lookup: `iters` passes over the shuffled key order; returns ns per lookup.
 * hit(it, end) yields an accumulator contribution for each probe result. */
template <typename M, typename K, typename Hit>
static double bench_lookup(M& m, const std::vector<K>& keys, const std::vector<size_t>& order, int iters, Hit hit) {
    u64 acc = 0;
    double t0 = now_ns();
    for (int i = 0; i < iters; ++i) {
        for (size_t j = 0; j < order.size(); ++j) {
            auto it = m.find(keys[order[j]]);
            acc += hit(it, m.end());
        }
    }
    double t1 = now_ns();
    g_sink += acc;
    return (t1 - t0) / ((double)iters * (double)order.size());
}

template <typename K, typename V, typename MkKeys>
static void run_section(const size_t sizes[], int nsizes, MkKeys mk_keys) {
    using RMap = hash_map<K, V>;
    using SMap = std::unordered_map<K, V>;

    printf("%-8s | %-20s | %-20s | %-20s | %-20s | %-20s | %-20s\n", "size", "robin fill", "std fill", "robin find", "std find", "robin miss", "std miss");
    for (int s = 0; s < nsizes; ++s) {
        size_t n = sizes[s];

        auto keys  = mk_keys(n, 0);
        auto misses = mk_keys(n, 3 * n);
        auto vals  = make_vals(n);
        auto order = make_order(n);

        // Build both maps (unmeasured).
        RMap r;
        SMap q;
        for (size_t i = 0; i < n; ++i) {
            r[keys[i]] = vals[i];
            q[keys[i]]  = vals[i];
        }

        int passes = clampi(6000000LL / (long long)n, 1, 4000);
        int iters  = clampi(3000000LL / (long long)n, 2, 2000);

        double rf = bench_fill<RMap, K, V>(keys, vals, passes);
        double qf = bench_fill<SMap, K, V>(keys, vals, passes);
        double rh = bench_lookup(r, keys, order, iters, [](auto& it, auto end) -> u64 { return it == end ? 0u : (*it)[int_c<1>]; });
        double qh = bench_lookup(q, keys, order, iters, [](auto& it, auto end) -> u64 { return it == end ? 0u : (u64)it->second; });
        double rm = bench_lookup(r, misses, order, iters, [](auto& it, auto end) -> u64 { return it != end; });
        double qm = bench_lookup(q, misses, order, iters, [](auto& it, auto end) -> u64 { return it != end; });

        printf("%-8zu | %17.2f ns | %17.2f ns | %17.2f ns | %17.2f ns | %17.2f ns | %17.2f ns\n",
               n, rf, qf, rh, qh, rm, qm);
        printf("  (robin: %zu buckets, %.1f bytes/entry | std: %zu buckets, lf=%.2f)\n",
               r.capacity(), (double)r.raw_data().size() * (double)sizeof(robin_hash_bucket<K, V>) / (double)n,
               q.bucket_count(), q.load_factor());
    }
}

int main() {
    const size_t u64_sizes[] = {1024, 4096, 16384, 65536, 262144, 1048576};
    const size_t str_sizes[] = {1024, 4096, 16384, 65536, 262144, 1048576};

    printf("== u64 -> u64 ==\n");
    run_section<u64, u64>(u64_sizes, 6, [](size_t n, size_t off) { return make_u64_keys(n, off); });

    printf("\n== std::string -> u64 ==\n");
    run_section<std::string, u64>(str_sizes, 6, [](size_t n, size_t off) { return make_str_keys(n, off); });

    const size_t long_str_sizes[] = {1024, 4096, 16384, 65536};

    printf("\n== long std::string (96 bytes) -> u64 ==\n");
    run_section<std::string, u64>(long_str_sizes, 4, [](size_t n, size_t off) { return make_long_str_keys(n, off); });

    printf("\nsink=%llx\n", (unsigned long long)g_sink);
    return 0;
}
