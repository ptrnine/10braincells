#include <catch2/catch_test_macros.hpp>

#include <core/robin_map.hpp>
#include <core/hash.hpp>
#include <core/utility/move.hpp>

#include <string>
#include <unordered_map>

using namespace core;

namespace {
// Deterministic LCG so the fuzz test is reproducible.
struct lcg {
    u32 state = 0x12345678u;
    u32 next() {
        state = state * 1664525u + 1013904223u;
        return state;
    }
};

// Key type whose hash is always 0: every key lands in the same probe
// sequence, forcing maximal robin distances and fully clustered rehashes.
struct colliding_key {
    u32 id;
};
constexpr bool operator==(const colliding_key& a, const colliding_key& b) {
    return a.id == b.id;
}
} // namespace

template <>
struct core::hash_impl<colliding_key> {
    constexpr u64 operator()(const colliding_key&) const {
        return 0;
    }
};

namespace {
// Counts constructed/destroyed values; the difference must always equal the
// map size, so rehashes may not leak or double-destroy values.
struct counting_value {
    static inline i64 constructed = 0;
    static inline i64 destroyed   = 0;

    static void reset() {
        constructed = 0;
        destroyed   = 0;
    }

    counting_value() {
        ++constructed;
    }
    explicit counting_value(std::string s): v(mov(s)) {
        ++constructed;
    }
    counting_value(const counting_value& o): v(o.v) {
        ++constructed;
    }
    counting_value(counting_value&& o) noexcept: v(mov(o.v)) {
        ++constructed;
    }
    counting_value& operator=(const counting_value& o) {
        v = o.v;
        return *this;
    }
    counting_value& operator=(counting_value&& o) noexcept {
        v = mov(o.v);
        return *this;
    }
    ~counting_value() {
        ++destroyed;
    }

    std::string v;
};

// Allocator that counts how many times the bucket vector asked for memory.
struct counting_alloc : std::allocator<robin_hash_bucket<u32, i32>> {
    static inline size_t allocations = 0;

    counting_alloc() = default;
    counting_alloc(const counting_alloc&): std::allocator<robin_hash_bucket<u32, i32>>() {}
    template <class T>
    struct rebind {
        using other = counting_alloc;
    };

    robin_hash_bucket<u32, i32>* allocate(size_t n) {
        ++allocations;
        return std::allocator<robin_hash_bucket<u32, i32>>::allocate(n);
    }
    void deallocate(robin_hash_bucket<u32, i32>* p, size_t n) {
        std::allocator<robin_hash_bucket<u32, i32>>::deallocate(p, n);
    }
};
} // namespace

TEST_CASE("hash_map lazy allocation") {
    hash_map<u64, i32> m;

    SECTION("empty map does nothing") {
        CHECK(m.empty());
        CHECK(m.size() == 0);
        CHECK(m.capacity() == 0);
        CHECK(m.begin() == m.end());
        CHECK(!m.contains(0));
        CHECK(m.find(0) == m.end());
        CHECK(!m.erase(0));
        m.clear(); // no-op before allocation
        CHECK(m.capacity() == 0);
    }

    SECTION("allocates on first insert") {
        size_t capacity_before = m.capacity();
        REQUIRE(capacity_before == 0);

        m.emplace(1, 100);

        CHECK(m.size() == 1);
        CHECK(m.capacity() == 8); // initial_capacity
        CHECK(m.find(1) != m.end());
        CHECK(m.at(1) == 100);
    }
}

TEST_CASE("hash_map capacity invariants") {
    hash_map<u64, i32> m;

    // The capacity sequence is 8, 16, 32, ... = 2^k (power of two so dynamic
    // indexing is an and-mask), always >= size and never throwing (unlike the
    // static maps).
    for (size_t i = 0; i < 2000; ++i) {
        m.emplace(i, i32(i));

        size_t cap = m.capacity();
        CHECK(cap >= m.size());
        CHECK(cap >= 8);
        CHECK((cap & (cap - 1)) == 0); // power of two
        CHECK(m.at(i) == i32(i));
    }
}

TEST_CASE("hash_map rehash preserves contents") {
    hash_map<std::string, std::string> m;

    for (int i = 0; i < 1000; ++i) {
        std::string key = "k" + std::to_string(i);
        std::string val = "v" + std::to_string(i);
        m.emplace(key, val);

        // Re-verify everything at each table growth boundary.
        if (i % 32 == 31) {
            CHECK(m.size() == (size_t)(i + 1));
            for (int j = 0; j <= i; ++j) {
                std::string jkey = "k" + std::to_string(j);
                CHECK(m.contains(jkey));
                CHECK(m.at(jkey) == "v" + std::to_string(j));
            }
        }
    }

    CHECK(m.size() == 1000);
    // Erase every other key and verify survivors plus absences.
    for (int i = 0; i < 1000; i += 2) {
        CHECK(m.erase("k" + std::to_string(i)));
    }
    CHECK(m.size() == 500);
    for (int i = 0; i < 1000; ++i) {
        std::string key = "k" + std::to_string(i);
        if (i % 2 == 0)
            CHECK(!m.contains(key));
        else
            CHECK(m.at(key) == "v" + std::to_string(i));
    }
}

TEST_CASE("hash_map growth at 4/5 load") {
    // Dynamic maps grow when occupancy reaches 4/5 of capacity (they never
    // run to full occupancy — probe lengths in a full robin table are
    // unbounded). With the initial capacity of 8, the 8th insert (7*5 >=
    // 8*4) triggers growth to 16. Static maps throw robin_map_overflow
    // instead — see the static_hash_map tests.
    hash_map<u64, i32> m;

    for (size_t i = 0; i < 7; ++i)  // initial capacity is 8
        m.emplace(i, i32(i));
    REQUIRE(m.size() == 7);
    REQUIRE(m.capacity() == 8);

    // Overwriting an existing key must not throw and must not change the size.
    // (The table is already above the 4/5 threshold, so this emplace also
    // triggers growth to 16.)
    auto [it, inserted] = m.emplace(3, 999);
    CHECK_FALSE(inserted);
    CHECK(it.key() == 3);
    CHECK(it.value() == 3); // emplace() does not overwrite existing values
    CHECK(m.size() == 7);
    CHECK(m.capacity() == 16);

    m[3] = 999;
    CHECK(m.size() == 7);
    CHECK(m.at(3) == 999);

    m[5] = 777;
    CHECK(m.size() == 7);
    CHECK(m.at(5) == 777);

    // A brand-new key below the threshold does not grow the table.
    m.emplace(100, 1);
    CHECK(m.size() == 8);
    CHECK(m.capacity() == 16);
    CHECK(m.at(100) == 1);
    CHECK(m.at(3) == 999);
}

TEST_CASE("hash_map total hash collision") {
    hash_map<colliding_key, i32> m;

    // All keys hash to 0: the table fills as one long robin chain and every
    // rehash must rebuild it. 10000 keys => max distance ~10000 < u16 limit.
    for (u32 i = 0; i < 10000; ++i)
        m.emplace(colliding_key{i}, i32(i));
    REQUIRE(m.size() == 10000);

    for (u32 i = 0; i < 10000; ++i)
        CHECK(m.at(colliding_key{i}) == i32(i));
    CHECK_FALSE(m.contains(colliding_key{99999}));

    // Iteration visits every entry exactly once.
    {
        size_t count = 0;
        for (auto [key, value] : m) {
            (void)key;
            (void)value;
            ++count;
        }
        CHECK(count == 10000);
    }

    // Erase half the cluster and verify survivors/absences.
    for (u32 i = 0; i < 10000; i += 2)
        CHECK(m.erase(colliding_key{i}));
    CHECK(m.size() == 5000);
    for (u32 i = 0; i < 10000; ++i) {
        if (i % 2 == 0)
            CHECK_FALSE(m.contains(colliding_key{i}));
        else
            CHECK(m.at(colliding_key{i}) == i32(i));
    }
}

TEST_CASE("hash_map value accounting across rehash") {
    counting_value::reset();
    hash_map<u64, counting_value> m;

    for (size_t i = 0; i < 1000; ++i) {
        m.emplace(i, counting_value{"v" + std::to_string(i)});
        CHECK(counting_value::constructed - counting_value::destroyed == (i64)m.size());
    }

    for (size_t i = 0; i < 1000; i += 2) {
        m.erase(i);
        CHECK(counting_value::constructed - counting_value::destroyed == (i64)m.size());
    }

    m.clear();
    CHECK(counting_value::constructed - counting_value::destroyed == 0);

    // Reuse after clear(): insert again into the retained table.
    for (size_t i = 0; i < 2000; ++i) {
        m.emplace(i, counting_value{"x" + std::to_string(i)});
        CHECK(counting_value::constructed - counting_value::destroyed == (i64)m.size());
    }
    CHECK(m.at(1999).v == "x1999");
}

TEST_CASE("hash_map custom allocator") {
    counting_alloc::allocations = 0;

    hash_map<u32, i32, counting_alloc> m;
    CHECK(m.capacity() == 0);
    CHECK(counting_alloc::allocations == 0); // lazy: nothing allocated yet

    m.emplace(1, 1);
    CHECK(counting_alloc::allocations > 0);
    size_t after_first = counting_alloc::allocations;

    // Grow through several rehashes (8 -> 16 -> 32 -> 64 -> 128); every growth
    // must ask the allocator for memory.
    for (u32 i = 0; i < 100; ++i)
        m.emplace(i, i32(i));
    CHECK(counting_alloc::allocations > after_first);
    CHECK(m.size() == 100);
    CHECK(m.at(99) == 99);
}

TEST_CASE("hash_map copy and move") {
    hash_map<u64, std::string> m;
    for (size_t i = 0; i < 100; ++i)
        m.emplace(i, "v" + std::to_string(i));

    auto copy = m;
    CHECK(copy.size() == m.size());
    CHECK(copy.capacity() == m.capacity());
    for (size_t i = 0; i < 100; ++i) {
        CHECK(copy.at(i) == "v" + std::to_string(i));
        CHECK(copy.find(i) != copy.end());
    }

    // Mutating the copy does not affect the original.
    copy.emplace(999, "new");
    CHECK(copy.size() == 101);
    CHECK(m.size() == 100);
    CHECK_FALSE(m.contains(999));

    auto moved = mov(m);
    CHECK(moved.size() == 100);
    CHECK(moved.at(42) == "v42");
}

TEST_CASE("hash_map fuzz against unordered_map") {
    lcg     rng;
    hash_map<u64, i32> m;
    std::unordered_map<u64, i32> ref;

    for (int i = 0; i < 50000; ++i) {
        u64 key  = u64(rng.next());
        u32 op   = u32(rng.next() % 10);

        if (op < 5) {
            i32    val   = i32(rng.next());
            auto& slot = ref[key]; // insert/overwrite
            m.insert_or_assign(key, val);
            slot = val;
        }
        else if (op < 8) {
            bool erased = m.erase(key);
            CHECK(erased == (ref.erase(key) != 0));
        }
        else {
            CHECK(m.contains(key) == (ref.find(key) != ref.end()));
            if (ref.contains(key))
                CHECK(m.at(key) == ref.at(key));
        }
        CHECK(m.size() == ref.size());
    }

    // Final full comparison.
    for (const auto& [key, val] : ref) {
        CHECK(m.contains(key));
        CHECK(m.at(key) == val);
    }
    CHECK(m.size() == ref.size());
}
