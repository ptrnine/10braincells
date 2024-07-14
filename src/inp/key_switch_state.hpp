#pragma once

#include <inp/key_rt_state.hpp>

namespace inp {
template <auto low, auto high, sys::u32 on, sys::u32 off>
struct key_switch_state {
    static inline constexpr auto bits_count   = (sys::uint(high) - sys::uint(low)) + 1;
    static inline constexpr auto blocks_count = (bits_count - 1) / 64 + 1;
    static inline constexpr auto value_mask = (1U << on) | (1U << off);

    template <sys::u32 min, sys::u32 max, typename Block> requires (on >= min && on <= max) && (off >= min && off <= max)
    constexpr void update(const key_rt_state<low, high, min, max, Block>& key_state) {
        for (auto key : key_state) {
            auto [block, mask] = get_pos(key.key);

            switch (key.bits & value_mask) {
                case (1U << on):
                    state[block] |= mask;
                    break;
                case (1U << off):
                case value_mask:
                    state[block] &= ~mask;
                    break;
            }
        }
    }

    constexpr bool test(decltype(low) key) const {
        auto [block, mask] = get_pos(key);
        return state[block] & mask;
    }

    static constexpr core::tuple<sys::u32, sys::u64> get_pos(decltype(low) key) {
        auto idx = sys::u32(key) - sys::u32(low);
        auto block = idx / 64U;
        auto mask = sys::u64(1) << (idx % 64U);
        return {block, mask};
    }

    sys::u64 state[blocks_count] = {0};
};
} // namespace inp
