#include <catch2/catch_test_macros.hpp>

#include <core/robin_map.hpp>
#include <core/utility/move.hpp>

#include <string>
#include <unordered_map>
#include <unordered_set>

using namespace core;

namespace {
struct counting_value {
    static int constructed;
    static int destroyed;

    static void reset() {
        constructed = 0;
        destroyed   = 0;
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
int counting_value::constructed = 0;
int counting_value::destroyed   = 0;

struct move_only_value {
    move_only_value(): v(0) {}
    explicit move_only_value(int v_): v(v_) {}

    move_only_value(move_only_value&&)             = default;
    move_only_value& operator=(move_only_value&&)  = default;
    move_only_value(const move_only_value&)        = delete;
    move_only_value& operator=(const move_only_value&) = delete;

    int v;
};

struct obj_t {
    i32 x;
};

struct point_key {
    u32 x, y;
};
constexpr bool operator==(const point_key& a, const point_key& b) {
    return a.x == b.x && a.y == b.y;
}

// Deterministic LCG so the stress test is reproducible.
struct lcg {
    u32 state = 0x9E3779B9u;
    u32 next() {
        state = state * 1664525u + 1013904223u;
        return state;
    }
};
} // namespace

TEST_CASE("robin_map_bucket_base header") {
    SECTION("i32 keys") {
        robin_map_bucket<robin_map_bucket_base<i32, i32>, i32> b;
        CHECK(b.empty());
        CHECK(b.distance() == 0);

        b.init_header(3, -42);
        CHECK(!b.empty());
        CHECK(b.distance() == 3);
        CHECK(b.key() == -42);

        b.set_distance(7);
        CHECK(b.distance() == 7);
        CHECK(b.key() == -42);

        b.set_key(123);
        CHECK(b.key() == 123);
        CHECK(b.distance() == 7);

        b.construct_value(99);
        CHECK(b.value() == 99);
        b.destroy();
        CHECK(b.empty());
        CHECK(b.distance() == 0);
    }

    SECTION("u8 key width") {
        robin_map_bucket<robin_map_bucket_base<u8, i32>, i32> b;
        b.init_header(1, u8(0));
        // Key 0 is storable because distance is always >= 1.
        CHECK(!b.empty());
        CHECK(b.key() == u8(0));
        CHECK(b.distance() == 1);

        b.set_key(255);
        CHECK(b.key() == u8(255));
        b.destroy();
        CHECK(b.empty());
    }

    SECTION("u16 key width") {
        robin_map_bucket<robin_map_bucket_base<u16, i32>, i32> b;
        b.init_header(4, u16(65535));
        CHECK(b.key() == u16(65535));
        b.set_key(1);
        CHECK(b.key() == u16(1));
        b.destroy();
        CHECK(b.empty());
    }

    SECTION("u32 key width") {
        robin_map_bucket<robin_map_bucket_base<u32, i32>, i32> b;
        b.init_header(1, u32(0xDEADBEEF));
        CHECK(b.key() == u32(0xDEADBEEF));
        b.destroy();
        CHECK(b.empty());
    }

    SECTION("ptr key width (48 bits)") {
        robin_map_bucket<robin_map_bucket_base<int*, i32>, i32> b;
        auto k1 = reinterpret_cast<int*>(1ull << 40);
        auto k2 = reinterpret_cast<int*>(2ull << 40);
        b.init_header(2, k1);
        CHECK(b.key() == k1);
        CHECK(b.distance() == 2);
        b.set_key(k2);
        CHECK(b.key() == k2);
        CHECK(b.distance() == 2);
        b.destroy();
        CHECK(b.empty());
    }
}

TEST_CASE("robin_map_bucket values and ca_traits") {
    using bucket_t = robin_map_bucket<robin_map_bucket_base<u32, std::string>, std::string>;

    SECTION("construction from args") {
        bucket_t b(2, u32(777), std::string("hello"));
        CHECK(b.distance() == 2);
        CHECK(b.key() == u32(777));
        CHECK(b.value() == "hello");
    }

    SECTION("construct_value") {
        bucket_t b;
        b.init_header(1, u32(5));

        b.construct_value(std::string("first"));
        CHECK(b.value() == "first");

        b.destroy();
        CHECK(b.empty());

        b.construct_value(std::string("third"));  // construct again after destroy
        CHECK(b.value() == "third");
    }

    SECTION("set_value") {
        // NOTE: set_value assumes the bucket is in a genuinely empty state
        // (uninitialized value) when it takes the construct path; combining it
        // with init_header without a prior construct is UB (assign into
        // garbage). The construct path requires the header to be unset.
        bucket_t b;

        b.set_value(std::string("first"));  // construct path (empty header)
        CHECK(b.value() == "first");

        b.init_header(1, u32(5));
        b.set_value(std::string("second"));  // assign path (bucket occupied)
        CHECK(b.value() == "second");
        CHECK(b.key() == u32(5));

        b.destroy();
        CHECK(b.empty());

        b.set_value(std::string("third"));  // construct again after destroy
        CHECK(b.value() == "third");
    }

    SECTION("copy and move") {
        bucket_t a;
        a.init_header(3, u32(11));
        a.construct_value(std::string("aa"));

        bucket_t b(a);  // copy ctor
        CHECK(b.empty() == false);
        CHECK(b.key() == u32(11));
        CHECK(b.distance() == 3);
        CHECK(b.value() == "aa");
        CHECK(a.value() == "aa");

        bucket_t c;
        c = a;  // copy assign into empty
        CHECK(c.key() == u32(11));
        CHECK(c.value() == "aa");

        c = b;  // copy assign occupied <- occupied
        CHECK(c.value() == "aa");

        bucket_t d(mov(b));  // move ctor
        CHECK(d.key() == u32(11));
        CHECK(d.value() == "aa");
        CHECK(b.empty());

        bucket_t e;
        e = mov(d);  // move assign into empty
        CHECK(e.value() == "aa");
        CHECK(d.empty());

        c = mov(e);  // move assign occupied <- occupied
        CHECK(c.value() == "aa");
        CHECK(e.empty());

        bucket_t f;
        f = c;  // copy assign into empty: f becomes a copy of c
        CHECK(f.empty() == false);
        CHECK(f.key() == u32(11));
        CHECK(f.value() == "aa");
        CHECK(c.value() == "aa");
    }

    SECTION("swap") {
        bucket_t a, b;
        a.init_header(2, u32(1));
        a.construct_value(std::string("one"));
        b.init_header(5, u32(2));
        b.construct_value(std::string("two"));

        swap(a, b);
        CHECK(a.key() == u32(2));
        CHECK(a.distance() == 5);
        CHECK(a.value() == "two");
        CHECK(b.key() == u32(1));
        CHECK(b.distance() == 2);
        CHECK(b.value() == "one");

        bucket_t c;
        swap(a, c);  // swap with empty
        CHECK(a.empty());
        CHECK(!c.empty());
        CHECK(c.key() == u32(2));
        CHECK(c.value() == "two");
    }
}

TEST_CASE("robin_map_hashes") {
    SECTION("int_identity_hash") {
        CHECK(int_identity_hash<u32>{}(u32(5)) == 5);
        CHECK(int_identity_hash<u8>{}(u8(200)) == 200);
        // signed keys are sign-extended to u64 (not zero-extended)
        CHECK(int_identity_hash<i8>{}(i8(-1)) == u64(i8(-1)));
        CHECK(int_identity_hash<i32>{}(i32(-1)) == u64(i32(-1)));
    }

    SECTION("ptr_hash") {
        static i32 a[8];
        u64 h0 = ptr_hash<i32*>{}(a);
        CHECK(h0 == (u64)a / alignof(i32));
        CHECK(ptr_hash<i32*>{}(a + 1) == h0 + 1);
    }
}

TEST_CASE("static_int_map") {
    static_int_map<i32, i32, 4> m;
    static_assert(m.capacity() == 4);

    SECTION("sequential") {
        for (i32 k = 0; k < 4; ++k)
            m[k] = k * 10;

        i32 i = 0;
        for (auto&& [k, v] : m) {
            CHECK(k == i);
            CHECK(v == i * 10);
            ++i;
        }
        CHECK(i == 4);
    }

    SECTION("collisions") {
        // Identity hash: all keys are multiples of the capacity => same slot.
        for (i32 k = 4; k <= 16; k += 4)
            m[k] = k;

        i32 i = 4;
        for (auto&& [k, v] : m) {
            CHECK(k == i);
            CHECK(v == i);
            i += 4;
        }
        CHECK(i == 20);
    }

    SECTION("robin hood shifts") {
        auto get_keys = [](auto&& m) {
            array<i32, 5> res;
            for (size_t i = 0; i < m.raw_data().size(); ++i)
                res[i] = m.raw_data()[i].key();
            return res;
        };
        auto get_dists = [](auto&& m) {
            array<u16, 5> res;
            for (size_t i = 0; i < m.raw_data().size(); ++i)
                res[i] = m.raw_data()[i].distance();
            return res;
        };

        CHECK(m.size() == 0);
        CHECK(m.empty());

        m.emplace(3, 1);
        CHECK(get_dists(m) == array<u16, 5>{0, 0, 0, 1, 1});
        CHECK(get_keys(m) == array<i32, 5>{0, 0, 0, 3, 0});
        CHECK(m.size() == 1);

        m.emplace(2, 2);
        CHECK(get_dists(m) == array<u16, 5>{0, 0, 1, 1, 1});
        CHECK(get_keys(m) == array<i32, 5>{0, 0, 2, 3, 0});
        CHECK(m.size() == 2);

        m.emplace(6, 3);
        CHECK(get_dists(m) == array<u16, 5>{2, 0, 1, 2, 1});
        CHECK(get_keys(m) == array<i32, 5>{3, 0, 2, 6, 0});
        CHECK(m.size() == 3);

        m.emplace(10, 4);
        CHECK(get_dists(m) == array<u16, 5>{3, 3, 1, 2, 1});
        CHECK(get_keys(m) == array<i32, 5>{10, 3, 2, 6, 0});
        CHECK(m.size() == 4);

        // Erasing back-shifts entries; the shifted entries must stay findable.
        CHECK(m.erase(10));
        CHECK(get_dists(m) == array<u16, 5>{2, 0, 1, 2, 1});
        CHECK(get_keys(m) == array<i32, 5>{3, 0, 2, 6, 0});
        CHECK(m.size() == 3);

        CHECK(m.erase(6));
        CHECK(get_dists(m) == array<u16, 5>{0, 0, 1, 1, 1});
        CHECK(get_keys(m) == array<i32, 5>{0, 0, 2, 3, 0});
        CHECK(m.size() == 2);

        CHECK(m.erase(2));
        CHECK(get_dists(m) == array<u16, 5>{0, 0, 0, 1, 1});
        CHECK(m.size() == 1);

        CHECK(m.erase(3));
        CHECK(get_dists(m) == array<u16, 5>{0, 0, 0, 0, 1});
        CHECK(m.size() == 0);
        CHECK(m.empty());

        CHECK(m.find(2) == m.end());
        CHECK(m.find(3) == m.end());
    }

    SECTION("find and contains") {
        m.emplace(3, 1);
        m.emplace(2, 2);
        m.emplace(6, 3);
        m.emplace(10, 4);

        auto f3  = m.find(3);
        auto f2  = m.find(2);
        auto f6  = m.find(6);
        auto f10 = m.find(10);
        CHECK(f3 != m.end());
        CHECK(f2 != m.end());
        CHECK(f6 != m.end());
        CHECK(f10 != m.end());
        CHECK(f3.value() == 1);
        CHECK(f2.value() == 2);
        CHECK(f6.value() == 3);
        CHECK(f10.value() == 4);

        CHECK(m.find(1) == m.end());
        CHECK(m.find(-1) == m.end());
        CHECK(m.find(11) == m.end());

        CHECK(m.contains(3));
        CHECK(m.contains(10));
        CHECK(!m.contains(1));

        // Values are mutable through the iterator.
        f3.value() = 11;
        CHECK(m.find(3).value() == 11);
    }

    SECTION("at") {
        m.emplace(3, 1);
        CHECK(m.at(3) == 1);
        m.at(3) = 42;
        CHECK(m.at(3) == 42);

        bool ok = false;
        try {
            (void)m.at(1);
        }
        catch (const robin_map_key_not_found& e) {
            ok        = true;
            std::string what = e.what();
            CHECK(what == "Key not found");
        }
        CHECK(ok);
    }

    SECTION("erase") {
        m.emplace(3, 1);
        m.emplace(2, 2);
        m.emplace(6, 3);
        m.emplace(10, 4);

        CHECK(!m.erase(1));
        CHECK(m.size() == 4);

        CHECK(m.erase(6));
        CHECK(m.size() == 3);
        CHECK(!m.erase(6));
        CHECK(m.size() == 3);
        CHECK(m.find(6) == m.end());
        CHECK(m.find(3) != m.end());
        CHECK(m.find(2) != m.end());
        CHECK(m.find(10) != m.end());

        // Erase by iterator position.
        auto it = m.find(10);
        REQUIRE(it != m.end());
        m.erase(it);
        CHECK(m.size() == 2);
        CHECK(m.find(10) == m.end());
    }

    SECTION("emplace and insert_or_assign") {
        auto [i1, ins1] = m.emplace(3, 1);
        auto [i2, ins2] = m.emplace(2, 2);
        CHECK(ins1);
        CHECK(ins2);
        CHECK(i1.value() == 1);
        CHECK(i2.value() == 2);

        auto [i3, ins3] = m.emplace(3, 100);  // duplicate key
        REQUIRE(!ins3);
        CHECK(i3.value() == 1);  // existing value untouched

        auto [i4, ins4] = m.insert_or_assign(3, 100);  // duplicate key
        REQUIRE(!ins4);
        CHECK(i4.value() == 100);
        CHECK(m.at(3) == 100);

        auto [i5, ins5] = m.insert_or_assign(1, 5);  // new key
        REQUIRE(ins5);
        CHECK(i5.value() == 5);

        auto& v = m[2];  // operator[]
        CHECK(v == 2);
        v = 22;
        CHECK(m.at(2) == 22);
        // References stay valid (fixed storage, no relocation).
        auto& v3 = m.at(3);
        m[4] = 4;
        CHECK(&v3 == &m.at(3));
    }

    SECTION("overflow") {
        for (i32 k = 0; k < 4; ++k)
            m[k] = k;

        bool threw = false;
        try {
            m.emplace(100, 7);
        }
        catch (const robin_map_overflow& e) {
            threw        = true;
            std::string what = e.what();
            CHECK(what == "Not enough space");
        }
        CHECK(threw);
        CHECK(m.size() == 4);

        // After freeing a slot, insertion works again.
        REQUIRE(m.erase(0));
        auto [it, ins] = m.emplace(100, 7);
        REQUIRE(ins);
        CHECK(it.value() == 7);
        CHECK(m.size() == 4);
        CHECK(m.at(100) == 7);
    }

    SECTION("clear") {
        static_int_map<i32, i32, 8> m;
        for (i32 k = 1; k <= 6; ++k)
            m[k] = k * 10;
        CHECK(m.size() == 6);

        m.clear();
        CHECK(m.empty());
        CHECK(m.size() == 0);
        for (i32 k = 1; k <= 6; ++k)
            CHECK(m.find(k) == m.end());

        // Reusable after clear.
        m[1] = 10;
        CHECK(m.size() == 1);
        CHECK(m.at(1) == 10);

        m.clear();
        m.clear();  // clearing an empty map is a no-op
        CHECK(m.empty());
    }

    SECTION("negative keys") {
        static_int_map<i32, i32, 8> m;
        i32 keys[] = {-1, -16, -33, 17, 41};
        for (i32 k : keys)
            m[k] = -k;
        for (i32 k : keys) {
            CHECK(m.contains(k));
            CHECK(m.at(k) == -k);
        }
        CHECK(m.size() == 5);
    }

    SECTION("u8 and u16 key widths") {
        static_int_map<u8, u32, 4> m8;
        for (u8 k = 0; k < 4; ++k)
            m8[k] = k;
        CHECK(m8.find(u8(0)) != m8.end());  // key 0 must be storable
        for (u8 k = 0; k < 4; ++k)
            CHECK(m8.at(k) == k);

        static_int_map<u16, u32, 4> m16;
        m16[u16(65535)] = 1;
        m16[u16(0)]     = 2;
        CHECK(m16.contains(u16(65535)));
        CHECK(m16.contains(u16(0)));
        CHECK(m16.at(u16(65535)) == 1);
        CHECK(m16.at(u16(0)) == 2);
    }

    SECTION("multi-arg value construction") {
        static_int_map<i32, std::pair<i32, i32>, 8> m;
        auto [it, ins] = m.emplace(3, 10, 20);
        REQUIRE(ins);
        CHECK(it.value().first == 10);
        CHECK(it.value().second == 20);
    }

    SECTION("value lifetime") {
        counting_value::reset();
        static_int_map<i32, counting_value, 8> m;

        // Invariant: live buckets each hold exactly one live value, and all
        // transient values (stack buckets, temporaries) balance out.
        auto check_balance = [&m] { CHECK(counting_value::destroyed == counting_value::constructed - m.size()); };

        m.emplace(1, std::string("a"));
        m.emplace(2, std::string("b"));
        m.emplace(1, std::string("a2"));  // duplicate: no new value
        CHECK(counting_value::constructed == 2);
        CHECK(counting_value::destroyed == 0);
        CHECK(m.at(1).v == "a");
        check_balance();

        m.insert_or_assign(1, std::string("a3"));  // temp value: +1 construct, +1 destroy
        CHECK(counting_value::constructed == 3);
        CHECK(counting_value::destroyed == 1);
        CHECK(m.at(1).v == "a3");
        check_balance();

        // Robin-hood relocation (keys 1, 9, 17 hash to the same slot) must
        // not leak or double-destroy values.
        m.emplace(9, std::string("c"));
        m.emplace(17, std::string("d"));
        check_balance();

        REQUIRE(m.erase(1));
        check_balance();
        m.clear();
        CHECK(m.empty());
        CHECK(counting_value::constructed == counting_value::destroyed);
    }

    SECTION("move-only values unsupported (current implementation)") {
        // For a move-only value with trivial move-assignment the bucket
        // move-assign is implicitly deleted: the ca_traits chain defaults to
        // member-wise assignment, which falls back to the deleted copy-assign.
        // robin_map's bucket swap / erase back-shift therefore cannot compile
        // for such values. Document the current limitation.
        using bucket_t = robin_map_bucket<robin_map_bucket_base<i32, move_only_value>, move_only_value>;
        static_assert(!assign<bucket_t&, bucket_t&&>);
        static_assert(!assign<bucket_t, bucket_t&&>);
    }
}

TEST_CASE("static_ptr_map (object pointers)") {
    static obj_t objects[32];

    SECTION("basic ops with real objects") {
        static_ptr_map<obj_t*, std::string, 4> m;
        m.emplace(&objects[0], "a");
        m.emplace(&objects[4], "b");
        m.emplace(&objects[8], "c");
        m.emplace(&objects[12], "d");

        CHECK(m.size() == 4);
        CHECK(m.at(&objects[8]) == "c");
        m.at(&objects[8]) = "C";
        CHECK(m.at(&objects[8]) == "C");
        CHECK(m.find(&objects[12]).key() == &objects[12]);
        CHECK(m.find(&objects[16]) == m.end());

        CHECK(m.erase(&objects[0]));
        CHECK(!m.erase(&objects[0]));
        CHECK(m.size() == 3);
        CHECK(m.find(&objects[4]) != m.end());
        CHECK(m.find(&objects[12]) != m.end());

        m.clear();
        CHECK(m.empty());
    }
}

TEST_CASE("robin_map_iterator") {
    static_int_map<i32, i32, 16> m;
    i32 keys[5] = {7, 23, 39, 55, 71};
    for (i32 k : keys)
        m[k] = k * 100;

    SECTION("begin/end") {
        static_int_map<i32, i32, 16> e;
        CHECK(e.begin() == e.end());
        CHECK(e.size() == 0);
        CHECK(m.begin() != m.end());
    }

    SECTION("traversal visits every entry exactly once") {
        std::unordered_set<i32> seen;
        auto prev = m.begin();
        bool first = true;
        for (auto it = m.begin(); it != m.end(); ++it) {
            if (!first)
                CHECK(prev < it);
            prev  = it;
            first = false;
            CHECK(it.value() == it.key() * 100);
            seen.insert(it.key());
        }
        for (i32 k : keys)
            CHECK(seen.count(k) == 1);
        CHECK(seen.size() == 5);
    }

    SECTION("dereference and accessors") {
        auto it = m.begin();
        auto [k, v] = *it;
        CHECK(it.key() == k);
        CHECK(it.value() == v);
        CHECK(it.pointer() != nullptr);
        CHECK(it.pointer()->key() == k);

        v = 12345;  // mutate through the tuple's value reference
        CHECK(it.value() == 12345);
        auto f = m.find(k);
        REQUIRE(f != m.end());
        CHECK(f.value() == 12345);
    }

    SECTION("post-increment") {
        auto it    = m.begin();
        auto k0    = it.key();
        auto prev  = it++;
        CHECK(prev.key() == k0);
        CHECK(prev < it);
        CHECK(it != m.end());
    }

    SECTION("end() is stable across insertions and erasures") {
        auto e = m.end();
        m[100] = 1;
        CHECK(m.end() == e);
        CHECK(m.erase(100));
        CHECK(m.end() == e);
    }
}

TEST_CASE("static_int_set") {
    SECTION("basic") {
        static_int_set<i32, 8> s;
        CHECK(s.empty());
        CHECK(s.size() == 0);

        auto [it, ins] = s.emplace(3);
        REQUIRE(ins);
        CHECK(s.size() == 1);
        CHECK(!s.empty());
        CHECK(s.contains(3));
        CHECK(!s.contains(4));
        CHECK(it.key() == 3);

        auto [it2, ins2] = s.emplace(3);  // duplicate
        REQUIRE_FALSE(ins2);
        CHECK(it2.key() == 3);
        CHECK(s.size() == 1);

        CHECK(s.find(3) != s.end());
        CHECK(s.find(4) == s.end());

        REQUIRE(s.erase(3));
        CHECK(!s.erase(3));
        CHECK(s.empty());
    }

    SECTION("iteration and clear") {
        static_int_set<i32, 16> s;
        i32 keys[] = {5, 9, 21, 30, 44};
        for (i32 k : keys)
            s.emplace(k);
        CHECK(s.size() == 5);

        std::unordered_set<i32> seen;
        for (auto it = s.begin(); it != s.end(); ++it)
            seen.insert(it.key());
        for (i32 k : keys)
            CHECK(seen.count(k) == 1);
        CHECK(seen.size() == 5);

        s.clear();
        CHECK(s.empty());
        CHECK(s.begin() == s.end());
    }

    SECTION("overflow") {
        static_int_set<i32, 4> s;
        for (i32 k = 0; k < 4; ++k)
            s.emplace(k);
        bool threw = false;
        try {
            s.emplace(100);
        }
        catch (const robin_map_overflow&) {
            threw = true;
        }
        CHECK(threw);
        CHECK(s.size() == 4);
    }

    SECTION("erase by iterator") {
        static_int_set<i32, 16> s;
        i32 keys[] = {5, 9, 21, 30};
        for (i32 k : keys)
            s.emplace(k);

        auto it = s.find(9);
        REQUIRE(it != s.end());
        s.erase(it);
        CHECK(s.size() == 3);
        CHECK(!s.contains(9));
        CHECK(s.contains(5));
        CHECK(s.contains(21));
        CHECK(s.contains(30));
    }
}

TEST_CASE("static_ptr_set") {
    static obj_t objects[16];

    static_ptr_set<obj_t*, 4> s;
    CHECK(s.empty());

    for (int i = 0; i < 4; ++i)
        s.emplace(&objects[i]);
    CHECK(s.size() == 4);

    CHECK(s.contains(&objects[2]));
    CHECK(!s.contains(&objects[4]));

    auto [it, ins] = s.emplace(&objects[2]);  // duplicate
    REQUIRE_FALSE(ins);
    CHECK(it.key() == &objects[2]);
    CHECK(s.size() == 4);

    // Full set: a new key overflows.
    bool threw = false;
    try {
        s.emplace(&objects[4]);
    }
    catch (const robin_map_overflow&) {
        threw = true;
    }
    CHECK(threw);
    CHECK(s.size() == 4);

    // Freeing a slot allows insertion again.
    REQUIRE(s.erase(&objects[1]));
    CHECK(!s.erase(&objects[1]));
    CHECK(s.size() == 3);
    auto [it4, ins4] = s.emplace(&objects[4]);
    REQUIRE(ins4);
    CHECK(it4.key() == &objects[4]);
    CHECK(s.size() == 4);

    s.clear();
    CHECK(s.empty());
}

TEST_CASE("static_int_map stress (oracle comparison)") {
    static_int_map<i32, i32, 1024> m;
    std::unordered_map<i32, i32> oracle;
    lcg rng;

    for (int op = 0; op < 100000; ++op) {
        i32 key = i32(rng.next() % 997);
        u32 action = rng.next() % 100;

        if (action < 45) {  // insert
            i32 v = i32(rng.next() % 100000);
            auto [it, ins] = m.emplace(key, v);
            if (oracle.count(key)) {
                CHECK_FALSE(ins);
                CHECK(it.value() == oracle[key]);
            }
            else {
                CHECK(ins);
                CHECK(it.value() == v);
                oracle[key] = v;
            }
        }
        else if (action < 75) {  // erase by key
            CHECK(m.erase(key) == (oracle.count(key) != 0));
            oracle.erase(key);
        }
        else {  // lookup
            auto it = m.find(key);
            if (oracle.count(key)) {
                CHECK(it != m.end());
                CHECK(it.value() == oracle[key]);
                CHECK(m.contains(key));
            }
            else {
                CHECK(it == m.end());
                CHECK_FALSE(m.contains(key));
            }
        }

        CHECK(m.size() == oracle.size());

        // Occasionally erase through an iterator.
        if (m.size() > 4 && rng.next() % 64 == 0) {
            auto it = m.begin();
            u32 steps = u32(rng.next() % m.size());
            for (u32 i = 0; i < steps; ++i)
                ++it;
            REQUIRE(it != m.end());
            i32 k = it.key();
            CHECK(m.contains(k));
            m.erase(it);
            oracle.erase(k);
            CHECK_FALSE(m.contains(k));
            CHECK(m.size() == oracle.size());
        }
    }

    // Final cross-check in both directions.
    for (auto&& [k, v] : m)
        CHECK((oracle.count(k) && oracle[k] == v));
    for (auto&& [k, v] : oracle)
        CHECK(m.at(k) == v);

    m.clear();
    CHECK(m.empty());
    CHECK(m.begin() == m.end());
}

TEST_CASE("static_hash_map (generic keys)") {
    SECTION("std::string keys") {
        static_hash_map<std::string, i32, 8> m;
        CHECK(m.empty());
        CHECK(m.size() == 0);

        m["apple"]  = 1;
        m["banana"] = 2;
        m["cherry"] = 3;
        CHECK(m.size() == 3);

        CHECK(m.contains("apple"));
        CHECK(m.contains("banana"));
        CHECK(!m.contains("durian"));

        CHECK(m.at("banana") == 2);
        CHECK(m["apple"] == 1);

        // Updating an existing key does not grow the map.
        m["apple"] = 10;
        CHECK(m.at("apple") == 10);
        CHECK(m.size() == 3);

        // emplace: duplicate keeps the old value, new inserts.
        auto [it1, ins1] = m.emplace("banana", 20);
        REQUIRE_FALSE(ins1);
        CHECK(it1.key() == "banana");
        CHECK(it1.value() == 2);

        auto [it2, ins2] = m.emplace("durian", 4);
        REQUIRE(ins2);
        CHECK(it2.key() == "durian");
        CHECK(it2.value() == 4);
        CHECK(m.size() == 4);

        // insert_or_assign: existing replaces, new inserts.
        auto [it3, ins3] = m.insert_or_assign("durian", 40);
        REQUIRE_FALSE(ins3);
        CHECK(m.at("durian") == 40);

        auto [it4, ins4] = m.insert_or_assign("elder", 5);
        REQUIRE(ins4);
        CHECK(it4.value() == 5);

        // Iteration visits every entry exactly once.
        i32  total = 0;
        size_t count = 0;
        for (auto&& [k, v] : m) {
            (void)k;
            total += v;
            ++count;
        }
        CHECK(count == 5);
        CHECK(total == 10 + 2 + 3 + 40 + 5);

        // Erase by key.
        REQUIRE(m.erase("banana"));
        CHECK(!m.erase("banana"));
        CHECK(!m.contains("banana"));
        CHECK(m.size() == 4);

        // Erase by iterator.
        auto found = m.find("cherry");
        REQUIRE(found != m.end());
        m.erase(found);
        CHECK(!m.contains("cherry"));
        CHECK(m.size() == 3);

        // at() throws for missing keys.
        bool threw = false;
        try {
            m.at("lemon");
        }
        catch (const robin_map_key_not_found&) {
            threw = true;
        }
        CHECK(threw);

        // Fill to capacity, then overflow (3 entries survived the erasures
        // above, so add 5 more to reach capacity 8).
        m["fig"]    = 7;
        m["grape"]  = 8;
        m["guava"]  = 9;
        m["honey"]  = 10;
        m["kiwi"]   = 11;
        CHECK(m.size() == 8);

        threw = false;
        try {
            m["lemon"] = 12;
        }
        catch (const robin_map_overflow&) {
            threw = true;
        }
        CHECK(threw);
        CHECK(m.size() == 8);

        // clear and reuse.
        m.clear();
        CHECK(m.empty());
        CHECK(m.size() == 0);
        CHECK(m.begin() == m.end());
        m["apple"] = 1;
        CHECK(m.size() == 1);
        CHECK(m.at("apple") == 1);
    }

    SECTION("u64 keys (too wide for the header word)") {
        static_hash_map<u64, i32, 4> m;

        u64 k1 = 0xFFFFFFFFFFFFFFFFu;
        m[k1]            = 1;
        m[u64(5)]        = 2;
        m[u64(1) << 40]  = 3;
        CHECK(m.size() == 3);

        CHECK(m.at(k1) == 1);
        CHECK(m.at(u64(5)) == 2);
        CHECK(m.at(u64(1) << 40) == 3);
        CHECK(m.contains(k1));
        CHECK(!m.contains(u64(1) << 41));

        REQUIRE(m.erase(k1));
        CHECK(!m.contains(k1));
        CHECK(m.size() == 2);
    }

    SECTION("trivial struct keys (byte hash)") {
        static_hash_map<point_key, i32, 4> m;

        m[point_key{1, 2}] = 10;
        m[point_key{2, 2}] = 20;
        m[point_key{1, 1}] = 30;
        CHECK(m.size() == 3);

        CHECK(m.at(point_key{1, 2}) == 10);
        CHECK(m.at(point_key{2, 2}) == 20);
        CHECK(!m.contains(point_key{2, 1}));

        // Duplicate emplace keeps the old value.
        auto [it, ins] = m.emplace(point_key{1, 2}, 99);
        REQUIRE_FALSE(ins);
        CHECK(it.value() == 10);

        m[point_key{1, 2}] = 11;
        CHECK(m.at(point_key{1, 2}) == 11);

        REQUIRE(m.erase(point_key{1, 1}));
        CHECK(m.size() == 2);
    }
}

TEST_CASE("static_hash_set (generic keys)") {
    static_hash_set<std::string, 4> s;
    CHECK(s.empty());

    s.emplace("a");
    s.emplace("bb");
    s.emplace("ccc");
    s.emplace("dddd");
    CHECK(s.size() == 4);

    CHECK(s.contains("ccc"));
    CHECK(!s.contains("ee"));

    // Duplicate insert is a no-op.
    auto [it, ins] = s.emplace("bb");
    REQUIRE_FALSE(ins);
    CHECK(it.key() == "bb");
    CHECK(s.size() == 4);

    // Full set: a new key overflows.
    bool threw = false;
    try {
        s.emplace("eeee");
    }
    catch (const robin_map_overflow&) {
        threw = true;
    }
    CHECK(threw);
    CHECK(s.size() == 4);

    // Freeing a slot allows insertion again.
    REQUIRE(s.erase("ccc"));
    CHECK(!s.erase("ccc"));
    CHECK(!s.contains("ccc"));
    CHECK(s.size() == 3);

    auto [it2, ins2] = s.emplace("eeee");
    REQUIRE(ins2);
    CHECK(it2.key() == "eeee");
    CHECK(s.size() == 4);

    s.clear();
    CHECK(s.empty());
}

TEST_CASE("static_hash_map stress (string keys, oracle comparison)") {
    static_hash_map<std::string, i32, 64> m;
    std::unordered_map<std::string, i32> oracle;
    lcg  rng;

    for (int op = 0; op < 20000; ++op) {
        // Small key pool to force collisions.
        u32 key_id = rng.next() % 47;
        auto key   = std::string("key_") + std::to_string(key_id);
        u32 action  = rng.next() % 100;

        if (action < 45) {  // insert
            i32 v = i32(rng.next() % 100000);
            auto [it, ins] = m.emplace(key, v);
            if (oracle.count(key)) {
                CHECK_FALSE(ins);
                CHECK(it.value() == oracle[key]);
            }
            else {
                CHECK(ins);
                CHECK(it.value() == v);
                oracle[key] = v;
            }
        }
        else if (action < 75) {  // erase by key
            CHECK(m.erase(key) == (oracle.count(key) != 0));
            oracle.erase(key);
        }
        else {  // lookup
            auto it = m.find(key);
            if (oracle.count(key)) {
                CHECK(it != m.end());
                CHECK(it.value() == oracle[key]);
                CHECK(m.contains(key));
            }
            else {
                CHECK(it == m.end());
                CHECK_FALSE(m.contains(key));
            }
        }

        CHECK(m.size() == oracle.size());

        // Occasionally erase through an iterator.
        if (m.size() > 4 && rng.next() % 64 == 0) {
            auto it = m.begin();
            u32 steps = u32(rng.next() % m.size());
            for (u32 i = 0; i < steps; ++i)
                ++it;
            REQUIRE(it != m.end());
            std::string k = it.key();  // copy: the bucket is erased below
            CHECK(m.contains(k));
            m.erase(it);
            oracle.erase(k);
            CHECK_FALSE(m.contains(k));
            CHECK(m.size() == oracle.size());
        }
    }

    // Final cross-check in both directions.
    for (auto&& [k, v] : m)
        CHECK((oracle.count(k) && oracle[k] == v));
    for (auto&& [k, v] : oracle)
        CHECK(m.at(k) == v);

    m.clear();
    CHECK(m.empty());
    CHECK(m.begin() == m.end());
}
