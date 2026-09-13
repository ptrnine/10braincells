#pragma once

#include <grx/vk/descriptor_set.cg.hpp>
#include <grx/vk/device.cg.hpp>

namespace vk {
class descriptor_set_store_t {
public:
    descriptor_set_store_t() = default;

    descriptor_set_store_t(const device_t& idev, vk::descriptor_pool ipool, std::vector<vk::descriptor_set> ihandles):
        dev(&idev), pool(ipool), _handles(core::mov(ihandles)), f(dev->instance()) {}

    descriptor_set_store_t(descriptor_set_store_t&&) noexcept            = default;
    descriptor_set_store_t& operator=(descriptor_set_store_t&&) noexcept = default;

    std::span<const vk::descriptor_set> handles() const {
        return _handles;
    }

    ~descriptor_set_store_t() {
        if (!_handles.empty()) {
            dev->instance().logger().info("Destroy std::vector<vk::descriptor_set> handles={}", _handles);
            f[cmd::free_descriptor_sets].call(dev->handle(), pool, core::u32(_handles.size()), _handles.data());
        }
    }

    size_t size() const noexcept {
        return _handles.size();
    }

    descriptor_set_t operator[](size_t idx) const {
        return {*dev, _handles[idx]};
    }

private:
    const device_t*                 dev;
    vk::descriptor_pool             pool;
    std::vector<vk::descriptor_set> _handles;

    function_provider</* start */
                cmd::free_descriptor_sets_t,
                /* end */ core::null_t>
    f;
};

auto device_t::allocate_descriptor_sets(const vk::descriptor_set_allocate_info& allocate_info) const {
    return descriptor_set_store_t{*this, allocate_info.descriptor_pool, allocate_descriptor_sets_raw(allocate_info).value()};
}
} // namespace vk
