#pragma once

#include <string>

#include <core/array.hpp>
#include <core/exception.hpp>
#include <core/opt.hpp>
#include <core/traits/conditional.hpp>
#include <core/tuple.hpp>
#include <core/utility/move.hpp>
#include <core/traits/ca_traits.hpp>
#include <util/log.hpp>

#define fwd(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace core
{

class ptr_map_overflow : public core::exception {
public:
    ptr_map_overflow(std::string message): msg(mov(message)) {}

    const char* what() const noexcept override {
        return msg.data();
    }

private:
    std::string msg;
};

class ptr_map_key_not_found : public core::exception {
public:
    ptr_map_key_not_found(std::string message): msg(mov(message)) {}

    const char* what() const noexcept override {
        return msg.data();
    }

private:
    std::string msg;
};


template <typename BucketT, size_t MaxSize>
class ptr_map_iterator {
public:
    using K = decltype(declval<BucketT>().key());
    using V = remove_ref<decltype(declval<BucketT>().value())>;
    //conditional<is_const<BucketT>, const typename BucketT::value_type, typename BucketT::value_type>;

    ptr_map_iterator(int_const<MaxSize>): _ptr(nullptr) {}
    ptr_map_iterator(BucketT* ptr, int_const<MaxSize>): _ptr(ptr) {
        while (_ptr->empty())
            ++_ptr;
    }

    ptr_map_iterator& operator++() {
        ++_ptr;
        while (_ptr->empty())
            ++_ptr;
        return *this;
    }

    ptr_map_iterator operator++(int) {
        auto it = *this;
        ++(*this);
        return it;
    }

    tuple<K, V&> operator*() const {
        return {_ptr->key(), _ptr->value()};
    }

    auto operator<=>(const ptr_map_iterator& iterator) const = default;

    BucketT* pointer() const {
        return _ptr;
    }

private:
    BucketT* _ptr;
};

namespace details
{
    static constexpr u64 ptr_map_key_bits     = 48;
    static constexpr u64 ptr_map_key_mask     = ~u64(0) >> (64 - ptr_map_key_bits);
    static constexpr u64 ptr_map_max_distance = (~u64(0) >> ptr_map_key_bits) - 1;

    struct ptr_map_ca_traits {
        constexpr static void cc(auto&& it, const auto& bucket) {
            if (!bucket.empty()) {
                it.header = bucket.header;
                it.construct_value(bucket.value());
            }
        }
        constexpr static void mc(auto&& it, auto&& bucket) {
            if (!bucket.empty()) {
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
                if (!it_empty)
                    it.value() = fwd(bucket).value();
            }
            else {
                if (it_empty)
                    it.construct_value(fwd(bucket).value());
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

    template <typename K, typename V>
    struct ptr_map_bucket_base {
        constexpr ptr_map_bucket_base() = default;

        constexpr ptr_map_bucket_base(u16 distance, const K* key, auto&&... args) {
            init_header(distance, key);
            construct_value(fwd(args)...);
        }

        constexpr ~ptr_map_bucket_base() {
            destroy();
        }

        constexpr void init_header(u16 distance, const K* key) {
            //__builtin_printf("init header dist: %i\n", int(distance));
            header = (u64(distance) << ptr_map_key_bits) | ((u64)key & ptr_map_key_mask);
        }

        constexpr bool empty() const {
            return !header;
        }

        constexpr u16 distance() const {
            return u16(header >> ptr_map_key_bits);
        }

        constexpr K* key() {
            return (K*)(header & ptr_map_key_mask);
        }

        constexpr const K* key() const {
            return (K*)(header & ptr_map_key_mask);
        }

        constexpr void set_distance(u16 value) {
            //__builtin_printf("set_distance dist: %i\n", int(value));
            header = (u64(value) << ptr_map_key_bits) | (header & ptr_map_key_mask);
        }

        constexpr void set_key(const K* value) {
            header = (header & ~ptr_map_key_mask) | ((u64)value & ptr_map_key_mask);
        }

        constexpr auto&& value(this auto&& it) {
            return fwd(it)._value;
        }

        constexpr V& construct_value(auto&&... args) {
            return *::new(static_cast<void*>(&_value)) V(fwd(args)...);
        }

        constexpr void set_value(auto&& value) {
            if (empty())
                construct_value(fwd(value));
            else
                value() = fwd(value);
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

    template <typename K, typename V>
    using ptr_map_bucket = ca_traits<ptr_map_ca_traits, ptr_map_bucket_base<K, V>, V>;

    template <typename K, typename V>
    void swap(ptr_map_bucket<K, V>& lhs, ptr_map_bucket<K, V>& rhs) {
        auto tmp = mov(rhs);
        rhs = mov(lhs);
        lhs = mov(tmp);
    }
} // namespace details

template <typename K, typename V, size_t MaxSize>
class ptr_map {
public:
    using bucket_t = details::ptr_map_bucket<K, V>;

    static inline constexpr size_t max_distance =
        details::ptr_map_max_distance > MaxSize + 1 ? MaxSize + 1 : details::ptr_map_max_distance;

    ptr_map() {
        memset(_data.data(), 0, sizeof(_data));
        _data[MaxSize].set_distance(1);
    }

    constexpr auto begin(this auto&& it) {
        return ptr_map_iterator{it._data.data(), max_size()};
    }

    constexpr auto end(this auto&& it) {
        return ptr_map_iterator{it._data.data() + MaxSize};
    }

    constexpr auto emplace(const K* key, auto&&... args) {
        size_t idx = to_idx(key);
        size_t dist = 1;
        //__builtin_printf("\nINDEX: %zu  dist: %zu %zu\n", idx, _data[idx].distance(), dist);
        for (; dist != max_distance && _data[idx].distance() >= dist; idx = next_idx(idx), ++dist) {
            if (_data[idx].key() == key)
                return tuple{ptr_map_iterator{&_data[idx], max_size()}, false};
        }

        if (_occupied == MaxSize)
            throw ptr_map_overflow("Not enough space");

        if (_data[idx].empty()) {
            //__builtin_printf("\nINDEX: %zu dist: %zu\n", idx, dist);
            _data[idx].init_header(u16(dist), key);
            _data[idx].construct_value(fwd(args)...);
            //__builtin_printf("insert (%s) on pos %zu\n", _data[idx].value().data(), idx);
            ++_occupied;
            return tuple{ptr_map_iterator{&_data[idx], max_size()}, true};
        }

        bucket_t new_bucket{u16(dist), key, fwd(args)...};
        //__builtin_printf("swap new(%s) <-> old(%s)\n", new_bucket.value().data(), _data[idx].value().data());
        details::swap(new_bucket, _data[idx]);
        //new_bucket.swap(_data[idx]);
        //__builtin_printf("IDX: %zu\n", idx);
        for (size_t dist = new_bucket.distance() + 1, i = next_idx(idx);; i = next_idx(i)) {
            if (_data[i].empty()) {
                //__builtin_printf("insert (%s) on pos %zu\n", new_bucket.value().data(), i);
                _data[i] = mov(new_bucket);
                _data[i].set_distance(u16(dist));
                ++_occupied;
                return tuple{ptr_map_iterator{&_data[i], max_size()}, true};
            }
            else if (_data[i].distance() < dist) {
                details::swap(new_bucket, _data[i]);
                //__builtin_printf("insert (%s) on pos %zu(%s)\n", new_bucket.value().data(), i, _data[i].value().data());
                //new_bucket.swap(_data[i]);
                _data[i].set_distance(dist);
                dist = new_bucket.distance() + 1;
            }
            else {
                ++dist;
            }
        }
    }

    constexpr auto insert_or_assign(const K* key, auto&&... args) {
        auto [iterator, inserted] = emplace(key, fwd(args)...);
        if (!inserted)
            (*iterator)[int_c<1>] = V(fwd(args)...);
        return tuple{iterator, inserted};
    }

    constexpr bool erase(const K* key) {
        auto found = find(key);
        if (found == end())
            return false;

        erase(found);
        return true;
    }

    template <typename B>
    constexpr void erase(ptr_map_iterator<B, MaxSize> position) {
        auto idx = position.pointer() - _data.data();
        //util::glog().warn("remove at idx: {}", idx);
        position.pointer()->destroy();
        --_occupied;

        //util::glog().warn("next: {}", _data[next_idx(idx)].distance());
        for (auto next = next_idx(idx); _data[next].distance() > 1;) {
            _data[idx] = mov(_data[next]);
            _data[idx].set_distance(_data[idx].distance() - 1);
            idx = next;
            next = next_idx(next);
        }
    }

    constexpr V& operator[](const K* key) {
        return (*emplace(key))[int_c<1>];
    }

    constexpr auto& at(this auto&& it, const K* key) {
        auto found = it.find(key);
        if (found == it.end())
            throw ptr_map_key_not_found("Key not found");
        return (*found)[int_c<1>];
    }

    constexpr auto find(this auto&& it, const K* key) {
        for (size_t idx = to_idx(key), dist = 1; dist != max_distance && it._data[idx].distance() >= dist;
             idx = next_idx(idx), ++dist) {
            //__builtin_printf("bucket: %zu key: %p\n", idx, it._data[idx].key());
            if (it._data[idx].key() == key)
                return ptr_map_iterator{&it._data[idx], max_size()};
        }

        return it.end();
    }

    constexpr size_t size() const {
        return _occupied;
    }

    static constexpr int_const<MaxSize> max_size() {
        return {};

    }

    constexpr bool empty() const {
        return !_occupied;
    }

    constexpr void clear() {
        for (auto it = begin(); it != end(); ++it)
            erase(it);
    }

    auto& raw_data() const {
        return _data;
    }

private:
    static inline constexpr size_t next_idx(size_t idx) {
        return (idx + 1) % MaxSize;
    }

    static inline constexpr size_t to_idx(const K* ptr) {
        auto idx = (u64)ptr / alignof(K);
        return size_t(idx % MaxSize);
    }

private:
    array<bucket_t, MaxSize + 1> _data;
    size_t                       _occupied = 0;
};

} // namespace core

#undef fwd
