#pragma once

#include <core/tuple.hpp>
#include <inp/state_block.hpp>
#include <sys/event.hpp>

#define fwd(...) static_cast<decltype(__VA_ARGS__)>(__VA_ARGS__)

namespace inp
{
namespace details
{
    template <typename T>
    concept key_rt = core::any_of<T, sys::event_key_code, sys::event_button_code>;
}

template <typename KeyT>
struct key_tester {
    KeyT     key;
    sys::u32 bits;
};

template <auto klow, auto khigh, sys::u32 vmin = 0, sys::u32 vmax = 1, typename BlockInt = std::atomic<sys::u32>>
    requires(details::key_rt<decltype(klow)> && core::is_same<decltype(klow), decltype(khigh)>)
struct key_rt_state {
    using block_t         = state_block<BlockInt>;
    using block_element_t = block_t::type;

    static inline constexpr size_t block_element_size = block_t::type_size;
    static inline constexpr size_t value_bitcount     = (vmax - vmin) + 1;
    static inline constexpr size_t bitcount           = value_bitcount * ((size_t(khigh) - size_t(klow)) + 1);
    static inline constexpr size_t blockcount         = (bitcount - 1) / (block_element_size * 8) + 1;


    struct key_iterator {
        constexpr key_iterator(const key_rt_state* data, sys::u32 i): _data(data), _i(i) {
            while (_i <= sys::u32(khigh) && !operator*().bits)
                ++_i;
        }

        constexpr key_iterator& operator++() {
            ++_i;
            while (_i <= sys::u32(khigh) && !operator*().bits)
                ++_i;
            return *this;
        }

        void operator++(int) = delete;

        constexpr auto operator==(const key_iterator& rhs) const {
            return _i == rhs._i;
        }

        constexpr auto operator!=(const key_iterator& rhs) const {
            return !(*this == rhs);
        }

        constexpr auto operator*() const {
            sys::u32 res = 0;
            for (sys::u32 val = vmin; val <= vmax; ++val)
                res |= (_data->test(sys::event_key_code(_i), val) << val);
            return key_tester{decltype(klow)(_i), res};
        }

        const key_rt_state* _data;
        sys::u32          _i;
    };

    constexpr bool handle(details::key_rt auto key, sys::i32 value) {
        if (!is_value_valid(value))
            return false;

        if (!is_key_valid(key))
            return false;

        auto [blocknum, mask] = get_pos(key, value);

        blocks[blocknum].fetch_or(mask);
        return true;
    }

    constexpr bool handle(const sys::event& event) {
        return event.dispatch<bool>([&](decltype(klow) code, sys::i32 value) {
            return handle(code, value);
        });
    }

    constexpr bool test(details::key_rt auto key, sys::i32 value) const {
        if (!is_key_valid(key))
            return false;
        auto [blocknum, mask] = get_pos(key, value);
        return blocks[blocknum].load() & mask;
    }

    constexpr bool take(details::key_rt auto key, sys::i32 value) {
        if (!is_key_valid(key))
            return false;

        auto [blocknum, mask] = get_pos(key, value);
        if (!(blocks[blocknum].load() & mask))
            return false;

        blocks[blocknum].fetch_and(~mask);
        return true;
    }

    constexpr auto get_all() const {
        key_rt_state<klow, khigh, vmin, vmax, block_element_t> res;
        for (size_t i = 0; i < blockcount; ++i)
            res.blocks[i]._block = blocks[i].load();
        return res;
    }

    constexpr auto take_all() {
        key_rt_state<klow, khigh, vmin, vmax, block_element_t> res;
        for (size_t i = 0; i < blockcount; ++i)
            res.blocks[i]._block = blocks[i].exchange(0);
        return res;
    }

    constexpr key_iterator begin() const {
        return {this, sys::u32(klow)};
    }

    constexpr key_iterator end() const {
        return {this, sys::u32(khigh) + 1};
    }

    constexpr static bool is_key_valid(details::key_rt auto key) {
        return uint(key) >= uint(klow) && uint(key) <= uint(khigh);
    }

    constexpr static bool is_value_valid(sys::i32 key) {
        return key >= sys::u32(vmin) && key <= sys::u32(vmax);
    }

    constexpr static auto get_pos(details::key_rt auto key, sys::i32 value) {
        auto k        = (uint(key) - uint(klow)) * value_bitcount + (sys::u32(value) - vmin);
        auto blocknum = k % blockcount;
        auto bit = k / blockcount;
        return core::tuple{blocknum, block_element_t(1) << bit};
    }

    static constexpr bool is_atomic() {
        return block_t::is_atomic;
    }

    block_t blocks[blockcount];
};

template <typename T>
inline constexpr std::string to_string(key_tester<T> key) {
    std::string res;
    res += to_string(key.key);
    res += ':';
    res += std::to_string(key.bits);
    return res;
}

template <auto klow, auto khigh, sys::u32 vmin, sys::u32 vmax, typename BlockInt>
inline constexpr std::string to_string(const key_rt_state<klow, khigh, vmin, vmax, BlockInt>& rt) {
    std::string res;
    for (auto key : rt) {
        res += to_string(key);
        res += ' ';
    }
    if (!res.empty())
        res.pop_back();
    return res;
}
} // namespace inp

#undef fwd
