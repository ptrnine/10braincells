#pragma once

#include <core/int_const.hpp>

namespace tbc
{
enum class event_group {
    common,
    signal,
    input_device,
    socket,
};

inline constexpr auto evt_common       = core::int_c<event_group::common>;
inline constexpr auto evt_signal       = core::int_c<event_group::signal>;
inline constexpr auto evt_input_device = core::int_c<event_group::input_device>;
inline constexpr auto evt_socket       = core::int_c<event_group::socket>;
} // namespace tbc
