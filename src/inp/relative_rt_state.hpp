#pragma once

#include <core/meta/overload_resolution.hpp>
#include <core/meta/type_list.hpp>
#include <core/utility/int_seq.hpp>
#include <inp/state_block.hpp>
#include <sys/event.hpp>

namespace inp {

/**
 * @brief Accumulates relative values changes to be consumed from another thread
 *
 * For the instance, we can have separate thread with epoll on mouse device
 * that send relative movement events via handle() and graphical thread that consume
 * x/y mouse values via take() for camera movement
 *
 * @tparam codes - codes for handling
 */
template <typename BlockType = std::atomic<core::u32>, sys::event_relative_code... codes>
struct relative_rt_state {
    using block_t = state_block<BlockType>;
    using block_element_t = block_t::type;

    static constexpr bool is_atomic() {
        return block_t::is_atomic;
    }

    static inline constexpr auto code_to_idx = [] {
        core::array<size_t, sys::event_relative_codes_count> res = {0};
        core::type_list<core::int_const<codes>...>.indexed().foreach ([&](auto t) {
            auto code = core::decl_type<t.type()>{}.to_int();
            res[code] = t.index();
        });
        return res;
    }();

    constexpr bool handle(sys::event_relative_code code, sys::i32 value) {
        return core::idx_dispatch<sys::event_relative_codes_count>(size_t(code), [&](auto code) {
            if constexpr (requires { core::accept_var<size_t(codes)...>{code}; }) {
                auto idx = code_to_idx[code.value];
                _states[idx].fetch_add(uint(value));
                return true;
            }
            return false;
        });
    }

    constexpr bool handle(const sys::event& event) {
        return event.dispatch<bool>([&](sys::event_relative_code code, sys::i32 value) { return handle(code, value); });
    }

    template <sys::event_relative_code code> requires ((code == codes) || ...)
    constexpr auto get(core::int_const<code>) const {
        constexpr auto idx = code_to_idx[uint(code)];
        return (sys::i32)_states[idx].load();
    }

    template <sys::event_relative_code code> requires ((code == codes) || ...)
    constexpr auto take(core::int_const<code>) {
        constexpr auto idx = code_to_idx[uint(code)];
        return (sys::i32)_states[idx].exchange(0);
    }

    constexpr auto get_all() const {
        relative_rt_state<block_element_t, codes...> res;
        for (size_t i = 0; i < sizeof...(codes); ++i)
            res._states[i]._block = _states[i].load();
        return res;
    }

    constexpr auto take_all() {
        relative_rt_state<block_element_t, codes...> res;
        for (size_t i = 0; i < sizeof...(codes); ++i)
            res._states[i]._block = _states[i].exchange(0);
        return res;
    }

    block_t _states[sizeof...(codes)];
};
} // namespace inp
