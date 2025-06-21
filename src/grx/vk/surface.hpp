#pragma once

#include <grx/vk/instance.hpp>

namespace vk {
class surface_t {
public:
    surface_t(const instance_t& instance, surface_khr surface): inst(&instance), handle(surface) {
        f.pass_to([&](auto&... functions) {
            inst->load_functions_cached(functions...);
        });
    }

    ~surface_t() {
        if (handle.not_default()) {
            f[cmd::destroy_surface_khr].call(inst->handle(), handle, core::null);
        }
    }

    operator surface_khr() const {
        return handle;
    }

private:
    const instance_t*                            inst;
    core::moveonly_trivial<surface_khr, nullptr> handle;
    core::tuple<cmd::destroy_surface_khr_t>      f;
};
} // namespace vk
