#pragma once

//#define TBC_DSA_DEBUG

#include <string>
#include <utility>
#include <vector>

#include <core/array.hpp>
#include <core/concepts/assign.hpp>
#include <core/concepts/ctor.hpp>
#include <core/concepts/trivial_dtor.hpp>
#include <core/construct_at.hpp>
#include <core/exception.hpp>
#include <core/hash.hpp>
#include <core/macros.hpp>
#include <core/traits/add_const.hpp>
#include <core/traits/ca_traits.hpp>
#include <core/traits/conditional.hpp>
#include <core/traits/declval.hpp>
#include <core/traits/is_ptr.hpp>
#include <core/traits/remove_ptr.hpp>
#include <core/tuple.hpp>
#include <core/utility/move.hpp>

#define fwd(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace core
{
class robin_map_exception : public exception {
public:
    robin_map_exception(std::string message): msg(mov(message)) {}
    const char* what() const noexcept override { return msg.data(); }

private:
    std::string msg;
};

class robin_map_overflow : public robin_map_exception {
public:
    robin_map_overflow(std::string message): robin_map_exception(message) {}
};

class robin_map_key_not_found : public robin_map_exception {
public:
    robin_map_key_not_found(std::string message): robin_map_exception(message) {}
};

template <typename BucketT>
class robin_map_iterator {
public:
    using K           = decltype(declval<BucketT>().key());
    using const_key_t = decltype(declval<const BucketT>().key());
    using V           = remove_ref<decltype(declval<BucketT>().value())>;

    // _end is only a bound, never dereferenced. (nullptr, nullptr) is the
    // valid "no buckets" state for unallocated dynamic maps.
    constexpr robin_map_iterator(): _ptr(nullptr), _end(nullptr) {}
    constexpr robin_map_iterator(BucketT* ptr, BucketT* end): _ptr(ptr), _end(end) {
        while (_ptr != _end && _ptr->empty())
            ++_ptr;
    }

    constexpr robin_map_iterator& operator++() {
        ++_ptr;
        while (_ptr != _end && _ptr->empty())
            ++_ptr;
        return *this;
    }

    constexpr robin_map_iterator operator++(int) {
        auto it = *this;
        ++(*this);
        return it;
    }

    constexpr tuple<K, V&> operator*() const {
        return {_ptr->key(), _ptr->value()};
    }

    constexpr const_key_t key() const {
        return static_cast<const BucketT*>(_ptr)->key();
    }

    constexpr auto& value(this auto&& it) {
        return it._ptr->value();
    }

    constexpr auto operator<=>(const robin_map_iterator& iterator) const = default;

    constexpr BucketT* pointer() const {
        return _ptr;
    }

private:
    BucketT* _ptr;
    BucketT* _end;
};

struct robin_map_bucket_ca_traits {
    constexpr static void cc(auto&& it, const auto& bucket) {
        if (!bucket.empty()) {
            it.construct_key(bucket);
            it.header = bucket.header;
            it.construct_value(bucket.value());
        }
    }
    constexpr static void mc(auto&& it, auto&& bucket) {
        if (!bucket.empty()) {
            it.construct_key(fwd(bucket));
            it.header = bucket.header;
            it.construct_value(mov(bucket.value()));
            bucket.destroy();
        }
    }
    constexpr static auto&& assign(auto&& it, auto&& bucket) {
        auto it_empty     = it.empty();
        auto bucket_empty = bucket.empty();

        it.header = bucket.header;

        if (it_empty == bucket_empty) {
            if (!it_empty) {
                it.copy_key(fwd(bucket));
                it.value() = fwd(bucket).value();
            }
        }
        else {
            if (it_empty) {
                it.construct_key(fwd(bucket));
                it.construct_value(fwd(bucket).value());
            }
            else
                it.destroy();
        }

        return it;
    }
    constexpr static auto&& ca(auto&& it, const auto& bucket) {
        return assign(it, bucket);
    }
    constexpr static auto&& ma(auto&& it, auto&& bucket) {
        assign(it, mov(bucket));
        bucket.destroy();
        return it;
    }
};

using robin_map_distance_t = u16;

/*
 * Generic bucket: the key does not fit in the header word, so it lives in a
 * separate member and is hashed via core::hash (hash_impl<K>). K must be
 * copyable and `==`-comparable; neither K nor V needs default-constructibility.
 */
template <typename K, typename V>
struct robin_map_bucket_base {
    using const_key_t = const K&;

    // Header: bits 47:32 distance (0 => empty), bit 15 key live, bit 14 value
    // live, bits 13:0 low key-hash bits (probe pre-filter).
    static constexpr u64 key_live_mask   = 1ull << 15;
    static constexpr u64 value_live_mask = 1ull << 14;
    static constexpr u64 live_mask       = key_live_mask | value_live_mask;
    static constexpr u64 hash_bits_mask  = (1ull << 14) - 1;

    // Keys do not travel with the header word, so memberwise copy/move/assign
    // are handled explicitly.
    constexpr robin_map_bucket_base() = default;

    constexpr robin_map_bucket_base(u16 distance, const_key_t key, auto&&... args) {
        init_header(distance, key);
        construct_value(fwd(args)...);
    }

    constexpr ~robin_map_bucket_base() {
        destroy();
    }

    constexpr robin_map_bucket_base(const robin_map_bucket_base& other) {
        header = other.header;
        if (other.key_live())
            construct_at(&_key, other._key);
        if (other.value_live())
            construct_at(&_value, other._value);
    }

    constexpr robin_map_bucket_base(robin_map_bucket_base&& other) {
        header = other.header;
        if (other.key_live())
            construct_at(&_key, fwd(other)._key);
        if (other.value_live())
            construct_at(&_value, fwd(other)._value);
    }

    template <typename Other>
    constexpr robin_map_bucket_base& assign(Other&& other) {
        auto const it_key_live   = key_live();
        auto const it_value_live = value_live();

        header = other.header;

        if (it_key_live == other.key_live()) {
            if (it_key_live)
                _key = fwd(other)._key;
        }
        else if (it_key_live) {
            if constexpr (!trivial_dtor<K>)
                _key.~K();
        }
        else
            construct_at(&_key, fwd(other)._key);

        if (it_value_live == other.value_live()) {
            if (it_value_live)
                _value = fwd(other)._value;
        }
        else if (it_value_live) {
            if constexpr (!trivial_dtor<V>)
                _value.~V();
        }
        else
            construct_at(&_value, fwd(other)._value);

        return *this;
    }

    constexpr robin_map_bucket_base& operator=(const robin_map_bucket_base& other) {
        return assign(other);
    }

    constexpr robin_map_bucket_base& operator=(robin_map_bucket_base&& other) {
        return assign(fwd(other));
    }

    constexpr void init_header(u16 distance, const_key_t key, u64 hash = 0) {
        construct_at(&_key, key);
        header = (u64(distance) << 32) | key_live_mask | ((hash & hash_bits_mask) | (header & value_live_mask));
    }

    // Relocation path: stamp low hash bits into an already-built header.
    constexpr void stamp_hash(u64 hash) {
        header |= hash & hash_bits_mask;
    }

    // A stored key equal to the probed key always matches, so this only
    // ever rejects other keys' buckets.
    constexpr bool hash_match(u64 hash) const {
        return (header & hash_bits_mask) == (hash & hash_bits_mask);
    }

    constexpr bool empty() const {
        return !header;
    }

    constexpr u16 distance() const {
        return u16(header >> 32);
    }

    constexpr bool key_live() const {
        return header & key_live_mask;
    }

    constexpr bool value_live() const {
        return header & value_live_mask;
    }

    constexpr K key() {
        return _key;
    }

    constexpr const K& key() const {
        return _key;
    }

    constexpr void set_distance(u16 value) {
        header = (u64(value) << 32) | (header & (live_mask | hash_bits_mask));
    }

    constexpr void set_key(const_key_t value) {
        _key = value;
    }

    // Key travels with a separate member here (the header-key specialization
    // provides no-ops instead).
    constexpr void construct_key(auto&& other) {
        construct_at(&_key, fwd(other)._key);
        header |= key_live_mask;
    }

    constexpr void copy_key(auto&& other) {
        _key = fwd(other)._key;
    }

    constexpr auto&& value(this auto&& it) {
        return fwd(it)._value;
    }

    constexpr auto construct_value(auto&&... args) {
        header |= value_live_mask;
        return *core::construct_at(&_value, fwd(args)...);
    }

    constexpr void set_value(auto&& new_value) {
        if (empty())
            construct_value(fwd(new_value));
        else
            value() = fwd(new_value);
    }

    constexpr void destroy() {
        if (key_live()) {
            if constexpr (!trivial_dtor<K>)
                _key.~K();
        }
        if (value_live()) {
            if constexpr (!trivial_dtor<V>)
                _value.~V();
        }
        header = 0;
    }

    u64 header = 0;
    union {
        char _key_init = {};
        K    _key;
    };
    union {
        char _init = {};
        V    _value;
    };
};

// Bucket for pointer and u32/u16/u8 keys: the key lives in the header word.
template <typename K, typename V> requires is_ptr<remove_cv<K>> || (integral<remove_cv<K>> && sizeof(K) <= 4)
struct robin_map_bucket_base<K, V> {
    static constexpr u64 ptr_map_key_bits     = is_ptr<remove_cv<K>> ? 48 : 32;
    static constexpr u64 ptr_map_key_mask     = ~u64(0) >> (64 - ptr_map_key_bits);
    static constexpr u64 ptr_map_max_distance = (~u64(0) >> ptr_map_key_bits) - 1;

    using const_key_t = conditional<is_ptr<remove_cv<K>>, add_const<remove_ptr<K>>*, const K>;

    constexpr robin_map_bucket_base() = default;

    constexpr robin_map_bucket_base(u16 distance, const_key_t key, auto&&... args) {
        init_header(distance, key);
        construct_value(fwd(args)...);
    }

    constexpr ~robin_map_bucket_base() {
        destroy();
    }

    constexpr void init_header(u16 distance, const_key_t key, u64 hash = 0) {
        (void)hash; // the key itself fills the header; no room for hash bits
        header = (u64(distance) << ptr_map_key_bits) | ((u64)key & ptr_map_key_mask);
    }

    // The key already lives in the header word.
    constexpr void stamp_hash(u64) {}
    constexpr bool hash_match(u64) const { return true; }

    constexpr bool empty() const {
        return !header;
    }

    constexpr u16 distance() const {
        return u16(header >> ptr_map_key_bits);
    }

    constexpr K key() {
        return (K)(header & ptr_map_key_mask);
    }

    constexpr const_key_t key() const {
        return (const_key_t)(header & ptr_map_key_mask);
    }

    constexpr void set_distance(u16 value) {
        header = (u64(value) << ptr_map_key_bits) | (header & ptr_map_key_mask);
    }

    constexpr void set_key(const_key_t value) {
        header = (header & ~ptr_map_key_mask) | ((u64)value & ptr_map_key_mask);
    }

    // The key travels with the header word.
    constexpr void construct_key(auto&&) {}
    constexpr void copy_key(auto&&) {}

    constexpr auto&& value(this auto&& it) {
        return fwd(it)._value;
    }

    constexpr auto construct_value(auto&&... args) {
        return core::construct_at(&_value, fwd(args)...);
    }

    constexpr void set_value(auto&& new_value) {
        if (empty())
            construct_value(fwd(new_value));
        else
            value() = fwd(new_value);
    }

    constexpr void destroy() {
        if constexpr (!trivial_dtor<V>) {
            if (!empty())
                _value.~V();
        }
        header = 0;
    }

    u64 header = 0;
    union {
        char _init = {};
        V    _value;
    };
};

template <typename BucketT, typename V>
using robin_map_bucket = ca_traits<robin_map_bucket_ca_traits, BucketT, V>;

template <typename BucketT, typename V>
constexpr void swap(robin_map_bucket<BucketT, V>& lhs, robin_map_bucket<BucketT, V>& rhs) {
    auto tmp = mov(rhs);
    rhs = mov(lhs);
    lhs = mov(tmp);
}

template <typename T>
struct ptr_hash;

template <typename T>
struct ptr_hash<T*> {
    constexpr u64 operator()(const T* ptr) const {
        if constexpr (requires { alignof(T); })
            return (u64)ptr / alignof(T);
        else
            return (u64)ptr;
    }
};

template <typename T>
struct int_identity_hash;

template <typename T> requires integral<T>
struct int_identity_hash<T> {
    constexpr u64 operator()(T value) const {
        return u64(value);
    }
};

template <typename V, typename Container, typename Hash, robin_map_distance_t MaxDist = 0>
class robin_map_impl {
public:
    using bucket_t = remove_const_ref<decltype(declval<Container>()[0])>;
    using key_t = decltype(declval<bucket_t>().key());
    static constexpr bool have_static_storage = requires {Container::size();};

    // The last bucket slot (index capacity()) is never probed (next_idx wraps
    // around); it stays empty and serves as the iterator's end bound.

    constexpr auto begin(this auto&& it) {
        return robin_map_iterator{it.capacity() ? it._data.data() : nullptr,
                                   it.capacity() ? it._data.data() + it.capacity() : nullptr};
    }

    constexpr auto end(this auto&& it) {
        auto end_ptr = it.capacity() ? it._data.data() + it.capacity() : nullptr;
        return robin_map_iterator{end_ptr, end_ptr};
    }

    constexpr auto bucket_it(this auto&& it, size_t idx) {
        return robin_map_iterator{&it._data[idx], it._data.data() + it.capacity()};
    }

    constexpr auto emplace(auto&& key, auto&&... args) {
        auto hv = Hash{}(key);
        size_t idx = 0;
        size_t dist = 1;

        if constexpr (have_static_storage) {
            // Probe first: emplacing an existing key into a full static table
            // must succeed rather than throw.
            idx = to_idx(hv);
        }
        else {
            // Grow (or initialize) before probing. Never let the table run to
            // full occupancy — grow at 4/5 load.
            if (_data.empty() || _occupied * 5 >= capacity() * 4)
                rehash(_data.empty() ? initial_capacity : capacity() * 2);
            idx = to_idx(hv);
        }

        TBC_DSA_LOG("robin_map::emplace() idx: %zu bucket_dist: %zu dist: %zu\n", idx, _data[idx].distance(), dist);

        for (; dist != max_distance() && _data[idx].distance() >= dist; idx = next_idx(idx), ++dist) {
            if (_data[idx].hash_match(hv) && _data[idx].key() == key) {
                TBC_DSA_LOG("robin_map::emplace() found bucket => idx: %zu dist: %zu\n", idx, dist);
                return tuple{bucket_it(idx), false};
            }
        }

        if constexpr (have_static_storage) {
            TBC_DSA_LOG("robin_map::emplace() static storage check => occupied: %zu capacity: %zu\n", _occupied, capacity());
            if (_occupied == capacity())
                throw robin_map_overflow("Not enough space");
        }

        if (_data[idx].empty()) {
            TBC_DSA_LOG("robin_map::emplace() place in empty bucket => idx: %zu dist: %zu\n", idx, dist);

            _data[idx].init_header(u16(dist), key, hv);
            _data[idx].construct_value(fwd(args)...);
            ++_occupied;
            return tuple{bucket_it(idx), true};
        }

        TBC_DSA_LOG("robin_map::emplace() place instead of old => idx: %zu\n", idx);

        bucket_t new_bucket{u16(dist), key, fwd(args)...};
        new_bucket.stamp_hash(hv);
        swap(new_bucket, _data[idx]);

        for (size_t dist = new_bucket.distance() + 1, i = next_idx(idx);; i = next_idx(i)) {
            if (_data[i].empty()) {
                TBC_DSA_LOG("robin_map::emplace() relocate to empty bucket => idx: %zu\n", i);

                _data[i] = mov(new_bucket);
                _data[i].set_distance(u16(dist));
                ++_occupied;
                return tuple{bucket_it(idx), true};
            }
            else if (_data[i].distance() < dist) {
                TBC_DSA_LOG("robin_map::emplace() replace => idx: %zu\n", i);

                swap(new_bucket, _data[i]);
                _data[i].set_distance(dist);
                dist = new_bucket.distance() + 1;
            }
            else {
                ++dist;
            }
        }
    }

    constexpr auto insert_or_assign(auto&& key, auto&&... args) {
        auto [iterator, inserted] = emplace(fwd(key), fwd(args)...);
        if (!inserted)
            (*iterator)[int_c<1>] = V(fwd(args)...);
        return tuple{iterator, inserted};
    }

    constexpr bool erase(const auto& key) {
        auto found = find(key);
        if (found == end())
            return false;

        erase(found);
        return true;
    }

    template <typename B>
    constexpr void erase(robin_map_iterator<B> position) {
        auto idx = position.pointer() - _data.data();
        position.pointer()->destroy();
        --_occupied;

        for (auto next = next_idx(idx); _data[next].distance() > 1;) {
            _data[idx] = mov(_data[next]);
            _data[idx].set_distance(_data[idx].distance() - 1);
            _data[next].destroy(); // clears the source for trivially-assignable values
            idx = next;
            next = next_idx(next);
        }
    }

    constexpr V& operator[](auto&& key) {
        return (*emplace(fwd(key))[int_c<0>])[int_c<1>];
    }

    constexpr auto& at(this auto&& it, const auto& key) {
        auto found = it.find(key);
        if (found == it.end())
            throw robin_map_key_not_found("Key not found");
        return (*found)[int_c<1>];
    }

    constexpr auto find(this auto&& it, const auto& key) {
        auto hv = Hash{}(key);
        for (size_t idx = it.to_idx(hv), dist = 1; dist != it.max_distance() && it._data[idx].distance() >= dist;
             idx = it.next_idx(idx), ++dist) {
            if (it._data[idx].hash_match(hv) && it._data[idx].key() == key)
                return it.bucket_it(idx);
        }

        return it.end();
    }

    constexpr bool contains(const auto& key) const {
        return find(key) != end();
    }

    constexpr size_t size() const {
        return _occupied;
    }

    constexpr size_t capacity() const {
        if constexpr (have_static_storage)
            return Container::size() - 1;
        else
            return _data.empty() ? 0 : _data.size() - 1;
    }

    constexpr bool empty() const {
        return !_occupied;
    }

    constexpr void clear() {
        if (!_occupied)
            return;

        for (auto p = _data.data(), e = _data.data() + capacity(); p != e; ++p)
            p->destroy();

        _occupied = 0;
    }

    constexpr auto& raw_data() const {
        return _data;
    }

private:
    // Power of two, so dynamic indexing is a cheap and-mask.
    static constexpr size_t initial_capacity = 8;

    inline constexpr size_t max_distance() const {
        if constexpr (have_static_storage) {
            constexpr auto max_dist = limits<robin_map_distance_t>::max();
            return max_dist > Container::size() ? Container::size() + 1 : max_dist;
        }
        else
            return _data.size() + 1;
    }

    // Re-insert every live bucket of the old table. Insertion order does not
    // matter for the robin invariant.
    constexpr void rehash(size_t new_capacity) {
        Container fresh;
        fresh.resize(new_capacity + 1);
        std::swap(_data, fresh);
        _occupied = 0;

        for (auto p = fresh.data(), e = fresh.data() + (fresh.empty() ? 0 : fresh.size() - 1); p != e; ++p) {
            if (!p->empty()) {
                auto key = p->key();
                emplace(key, mov(p->value()));
                p->destroy();
            }
        }
    }

    inline constexpr size_t next_idx(size_t idx) const {
        if constexpr (have_static_storage)
            return (idx + 1) % capacity(); // static tables may have any size
        return (idx + 1) & (capacity() - 1); // dynamic capacity is a power of two
    }

    inline constexpr size_t to_idx(u64 hash) const {
        if constexpr (have_static_storage)
            return size_t(hash % capacity()); // static tables may have any size
        return size_t(hash & (capacity() - 1)); // dynamic capacity is a power of two
    }

private:
    Container _data = {};
    size_t    _occupied = 0;
};

struct robin_map_no_value {};

template <typename Container, typename Hash, robin_map_distance_t MaxDist = 0>
class robin_set_impl {
public:
    constexpr auto begin(this auto&& it) {
        return it._map.begin();
    }

    constexpr auto end(this auto&& it) {
        return it._map.end();
    }

    constexpr auto emplace(auto&& key) {
        return _map.emplace(key);
    }

    constexpr bool erase(const auto& key) {
        return _map.erase(key);
    }

    template <typename B>
    constexpr void erase(robin_map_iterator<B> position) {
        _map.erase(position);
    }

    constexpr auto find(this auto&& it, const auto& key) {
        return it._map.find(key);
    }

    constexpr bool contains(const auto& key) const {
        return _map.contains(key);
    }

    constexpr size_t size() const {
        return _map.size();
    }

    constexpr size_t capacity() const {
        return _map.capacity();
    }

    constexpr bool empty() const {
        return _map.empty();
    }

    constexpr void clear() {
        _map.clear();
    }

    auto& raw_data() const {
        return _map.raw_data();
    }

private:
    robin_map_impl<robin_map_no_value, Container, Hash, MaxDist> _map;
};

template <typename K, typename V, size_t MaxSize>
using static_ptr_map = robin_map_impl<V, array<robin_map_bucket<robin_map_bucket_base<K, V>, V>, MaxSize + 1>, ptr_hash<K>>;

template <typename K, size_t MaxSize>
using static_ptr_set = robin_set_impl<
    array<robin_map_bucket<robin_map_bucket_base<K, robin_map_no_value>, robin_map_no_value>, MaxSize + 1>,
    ptr_hash<K>>;

template <typename K, typename V, size_t MaxSize>
    requires(integral<K> && sizeof(K) <= 4)
using static_int_map =
    robin_map_impl<V, array<robin_map_bucket<robin_map_bucket_base<K, V>, V>, MaxSize + 1>, int_identity_hash<K>>;

template <typename K, size_t MaxSize>
    requires(integral<K> && sizeof(K) <= 4)
using static_int_set = robin_set_impl<
    array<robin_map_bucket<robin_map_bucket_base<K, robin_map_no_value>, robin_map_no_value>, MaxSize + 1>,
    int_identity_hash<K>>;

template <typename K, typename V, size_t MaxSize>
    requires(copy_ctor<K> && copy_assign<K>)
using static_hash_map =
    robin_map_impl<V, array<robin_map_bucket<robin_map_bucket_base<K, V>, V>, MaxSize + 1>, hash_impl<K>>;

template <typename K, size_t MaxSize>
    requires(copy_ctor<K> && copy_assign<K>)
using static_hash_set = robin_set_impl<
    array<robin_map_bucket<robin_map_bucket_base<K, robin_map_no_value>, robin_map_no_value>, MaxSize + 1>,
    hash_impl<K>>;

// Dynamic (std::vector-backed) storage; grows by doubling.
template <typename K, typename V>
using robin_hash_bucket = robin_map_bucket<robin_map_bucket_base<K, V>, V>;

template <typename K, typename V, typename Alloc = std::allocator<robin_hash_bucket<K, V>>>
    requires(copy_ctor<K> && copy_assign<K>)
using hash_map = robin_map_impl<V, std::vector<robin_hash_bucket<K, V>, Alloc>, hash_impl<K>>;
} // namespace core

#undef fwd
