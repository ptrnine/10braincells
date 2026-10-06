#pragma once


//#define TBC_DSA_DEBUG

#include <string>

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
class robin_map_exception : public core::exception {};

class robin_map_overflow : public robin_map_exception {
public:
    robin_map_overflow(std::string message): msg(mov(message)) {}

    const char* what() const noexcept override {
        return msg.data();
    }

private:
    std::string msg;
};

class robin_map_key_not_found : public robin_map_exception {
public:
    robin_map_key_not_found(std::string message): msg(mov(message)) {}

    const char* what() const noexcept override {
        return msg.data();
    }

private:
    std::string msg;
};

template <typename BucketT>
class robin_map_iterator {
public:
    using K           = decltype(declval<BucketT>().key());
    using const_key_t = decltype(declval<const BucketT>().key());
    using V           = remove_ref<decltype(declval<BucketT>().value())>;

    constexpr robin_map_iterator(): _ptr(nullptr) {}
    constexpr robin_map_iterator(BucketT* ptr): _ptr(ptr) {
        while (_ptr->empty())
            ++_ptr;
    }

    constexpr robin_map_iterator& operator++() {
        ++_ptr;
        while (_ptr->empty())
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
        // Const access so that non-trivial keys are not copied out.
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
};

struct robin_map_bucket_ca_traits {
    // Keys stored in the bucket itself (generic keys) are materialized via
    // construct_key() into an empty bucket and transferred via copy_key() into
    // an occupied one; keys stored in the header word are copied with the header.
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
 * Generic keys: keys that do not fit in the header word (neither pointers
 * nor small integrals) are stored in a separate `_key` member and hashed
 * via core::hash (hash_impl<K>). The header carries the distance plus live
 * flags for the key and the value.
 *
 * Requirements for K: copyable, comparable via `==`. Neither K nor V needs
 * to be default-constructible: the key and value live in unions and are
 * materialized (placement new) only while their live bit is set; an empty
 * bucket holds neither and destroy() destroys exactly the live members.
 */
template <typename K, typename V>
struct robin_map_bucket_base {
    using const_key_t = const K&;

    // Header layout: bits 47:32 hold the distance (0 => empty bucket), bit
    // 15 marks the key live, bit 14 marks the value live. The live bits let
    // a bucket carry a header (e.g. the map's sentinel bucket, which
    // terminates the iterator's empty-bucket skip) without holding a live
    // key or value.
    static constexpr u64 key_live_mask   = 1ull << 15;
    static constexpr u64 value_live_mask = 1ull << 14;
    static constexpr u64 live_mask       = key_live_mask | value_live_mask;

    // The key does not travel with the header word and memberwise copy/move
    // would clobber live keys/values, so the special members are provided
    // explicitly (the ca_traits wrapper still intercepts them for
    // non-trivial values via its Traits path).
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

    constexpr void init_header(u16 distance, const_key_t key) {
        construct_at(&_key, key);
        header = (u64(distance) << 32) | key_live_mask | (header & value_live_mask);
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
        header = (u64(value) << 32) | (header & live_mask);
    }

    constexpr void set_key(const_key_t value) {
        _key = value;
    }

    // Materialize the key in this bucket from another bucket (copy for
    // lvalues, move for rvalues). The header-key specialization provides a
    // no-op instead (its key travels with the header word).
    constexpr void construct_key(auto&& other) {
        construct_at(&_key, fwd(other)._key);
        header |= key_live_mask;
    }

    // Transfer the key from another bucket into this (key-live) bucket (copy
    // for lvalues, move for rvalues).
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

/* Implementation for pointer and u32/u16/u8 keys */
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

    constexpr void init_header(u16 distance, const_key_t key) {
        //__builtin_printf("init header dist: %i\n", int(distance));
        header = (u64(distance) << ptr_map_key_bits) | ((u64)key & ptr_map_key_mask);
    }

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
        //__builtin_printf("set_distance dist: %i\n", int(value));
        header = (u64(value) << ptr_map_key_bits) | (header & ptr_map_key_mask);
    }

    constexpr void set_key(const_key_t value) {
        header = (header & ~ptr_map_key_mask) | ((u64)value & ptr_map_key_mask);
    }

    // The key lives in the header word; copying the header transfers it.
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

/* TODO: implement for dynamic storage */
template <typename V, typename Container, typename Hash, robin_map_distance_t MaxDist = 0>
class robin_map_impl {
public:
    using bucket_t = remove_const_ref<decltype(declval<Container>()[0])>;
    using key_t = decltype(declval<bucket_t>().key());
    static constexpr bool have_static_storage = requires {Container::size();};

    //static inline constexpr size_t max_distance =
    //    details::ptr_map_max_distance > MaxSize + 1 ? MaxSize + 1 : details::ptr_map_max_distance;

    constexpr robin_map_impl() {
        // The sentinel bucket at index capacity() is never probed (next_idx
        // wraps modulo capacity()); its distance-1 header terminates the
        // iterator's empty-bucket skip. Generic buckets carry no live
        // key/value there (set_distance preserves the live bits, which are
        // unset).
        if constexpr (have_static_storage)
            _data[capacity()].set_distance(1);
    }

    constexpr auto begin(this auto&& it) {
        return robin_map_iterator{it._data.data()};
    }

    constexpr auto end(this auto&& it) {
        return robin_map_iterator{it._data.data() + it.capacity()};
    }


    constexpr auto emplace(auto&& key, auto&&... args) {
        size_t idx = to_idx(key);
        size_t dist = 1;

        TBC_DSA_LOG("robin_map::emplace() idx: %zu bucket_dist: %zu dist: %zu\n", idx, _data[idx].distance(), dist);

        for (; dist != max_distance() && _data[idx].distance() >= dist; idx = next_idx(idx), ++dist) {
            if (_data[idx].key() == key) {
                TBC_DSA_LOG("robin_map::emplace() found bucket => idx: %zu dist: %zu\n", idx, dist);
                return tuple{robin_map_iterator{&_data[idx]}, false};
            }
        }

        static_assert(have_static_storage);
        if constexpr (have_static_storage) {
            TBC_DSA_LOG("robin_map::emplace() static storage check => occupied: %zu capacity: %zu\n", _occupied, capacity());
            if (_occupied == capacity())
                throw robin_map_overflow("Not enough space");
        }

        if (_data[idx].empty()) {
            TBC_DSA_LOG("robin_map::emplace() place in empty bucket => idx: %zu dist: %zu\n", idx, dist);

            _data[idx].init_header(u16(dist), key);
            _data[idx].construct_value(fwd(args)...);
            ++_occupied;
            return tuple{robin_map_iterator{&_data[idx]}, true};
        }

        TBC_DSA_LOG("robin_map::emplace() place instead of old => idx: %zu\n", idx);

        bucket_t new_bucket{u16(dist), key, fwd(args)...};
        swap(new_bucket, _data[idx]);

        for (size_t dist = new_bucket.distance() + 1, i = next_idx(idx);; i = next_idx(i)) {
            if (_data[i].empty()) {
                TBC_DSA_LOG("robin_map::emplace() relocate to empty bucket => idx: %zu\n", i);

                _data[i] = mov(new_bucket);
                _data[i].set_distance(u16(dist));
                ++_occupied;
                // The new key was swapped into _data[idx]; _data[i] holds the
                // relocated (previously occupied) bucket.
                return tuple{robin_map_iterator{&_data[idx]}, true};
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
        auto idx = position.pointer() -_data.data();
        position.pointer()->destroy();
        --_occupied;

        for (auto next = next_idx(idx); _data[next].distance() > 1;) {
            _data[idx] = mov(_data[next]);
            _data[idx].set_distance(_data[idx].distance() - 1);
            // For trivially move-assignable values the bucket move-assign above
            // is a plain member-wise copy that leaves the source set; clear it.
            // (For non-trivial values the traits already destroyed it, so this
            // is a no-op.)
            _data[next].destroy();
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
        for (size_t idx = it.to_idx(key), dist = 1; dist != max_distance() && it._data[idx].distance() >= dist;
             idx = it.next_idx(idx), ++dist) {
            if (it._data[idx].key() == key)
                return robin_map_iterator{&it._data[idx]};
        }

        return it.end();
    }

    constexpr auto contains(const auto& key) const {
        return find(key) != end();
    }

    constexpr size_t size() const {
        return _occupied;
    }

    constexpr size_t capacity() const {
        if constexpr (have_static_storage)
            return Container::size() - 1;
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
    static constexpr size_t max_distance() {
        if constexpr (have_static_storage) {
            constexpr auto max_dist = limits<robin_map_distance_t>::max();
            return max_dist > Container::size() ? Container::size() + 1 : max_dist;
        } else
            return MaxDist;
    }

    inline constexpr size_t next_idx(size_t idx) const {
        return (idx + 1) % capacity();
    }

    inline constexpr size_t to_idx(const key_t& key) const {
        auto idx = Hash{}(key);
        return size_t(idx % capacity());
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

/*
 * Generic keys: keys that do not fit in the bucket header word (neither
 * pointers nor small integrals) are stored in the bucket itself and hashed
 * via core::hash (hash_impl<K>). K must be copyable and comparable via `==`
 * (default-constructibility is not required).
 */
template <typename K, typename V, size_t MaxSize>
    requires(copy_ctor<K> && copy_assign<K>)
using static_hash_map =
    robin_map_impl<V, array<robin_map_bucket<robin_map_bucket_base<K, V>, V>, MaxSize + 1>, hash_impl<K>>;

template <typename K, size_t MaxSize>
    requires(copy_ctor<K> && copy_assign<K>)
using static_hash_set = robin_set_impl<
    array<robin_map_bucket<robin_map_bucket_base<K, robin_map_no_value>, robin_map_no_value>, MaxSize + 1>,
    hash_impl<K>>;
} // namespace core

#undef fwd
