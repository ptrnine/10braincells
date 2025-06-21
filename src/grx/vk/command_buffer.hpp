#pragma once

#include <grx/vk/device.cg.hpp>
#include <grx/vk/command_buffer.cg.hpp>

namespace vk {
class command_buffer_store_t {
public:
    command_buffer_store_t(const device_t& idev, vk::command_pool ipool, std::vector<vk::command_buffer> ihandles):
        dev(&idev), pool(ipool), _handles(core::mov(ihandles)) {
        f.pass_to([&](auto&... functions) {
            dev->load_functions_cached(functions...);
        });
    }

    command_buffer_store_t(command_buffer_store_t&&) noexcept = default;
    command_buffer_store_t& operator=(command_buffer_store_t&&) noexcept = default;

    void load_functions_cached(auto&... functions) const {
        dev->load_functions_cached(functions...);
    }

    std::span<const vk::command_buffer> handles() const {
        return _handles;
    }

    ~command_buffer_store_t() {
        if (!_handles.empty()) {
            f[cmd::free_command_buffers].call(dev->handle(), pool, core::u32(_handles.size()), _handles.data());
        }
    }

    size_t size() const noexcept {
        return _handles.size();
    }

    command_buffer_t operator[](size_t idx) const {
        return {*dev, _handles[idx]};
    }

private:
    const device_t* dev;
    vk::command_pool pool;
    std::vector<vk::command_buffer> _handles;

    core::tuple</* start */
                cmd::free_command_buffers_t,
                /* end */ core::null_t>
    f;
};

auto device_t::allocate_command_buffers(const vk::command_buffer_allocate_info& allocate_info) const {
    return command_buffer_store_t{*this, allocate_info.command_pool, allocate_command_buffers_raw(allocate_info).value()};
}
} // namespace vk
