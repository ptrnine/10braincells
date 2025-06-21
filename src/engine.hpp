#pragma once

#include <input_mgr.hpp>
#include <core/epoller.hpp>

namespace tbc
{
static inline constexpr core::epoller_cfg epoller_config{
    .max_groups       = 8,
    .group_type       = core::type<tbc::event_group>,
    .id_type          = core::type<sys::u32>,
    .max_handler_size = 24,
};

class engine_object {
public:
    static engine_object& instance() {
        static engine_object obj;
        return obj;
    }

private:
    void init_input() {
        input_mgr.attach_to_epoller(event_polling);
    }

private:
    engine_object() {
        init_input();
    }

    ~engine_object() = default;

public:
    core::epoller_thread<epoller_config> event_polling;
    input_mgr<decltype(event_polling)>   input_mgr;
};

inline engine_object& engine() {
    return engine_object::instance();
}
} // namespace tbc
