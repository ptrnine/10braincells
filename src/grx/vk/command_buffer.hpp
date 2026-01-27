#pragma once

#include <grx/vk/device.cg.hpp>
#include <grx/vk/command_buffer.cg.hpp>

namespace vk {
class command_buffer_store_t {
public:
    command_buffer_store_t() = default;

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

class with_buffer {
public:
    with_buffer(command_buffer_t buffer, core::opt<command_buffer_reset_flags> reset = {}, const vk::command_buffer_begin_info& begin_info = {}):
        buff(buffer) {
        if (reset) {
            buff.reset(*reset).throws();
        }
        buff.begin(begin_info).throws();
    }

    with_buffer(with_buffer&&) = delete;
    with_buffer& operator=(with_buffer&&) = delete;

    ~with_buffer() noexcept(false) {
        buff.end().throws();
    }

    command_buffer_t& operator*() {
        return buff;
    }

    command_buffer_t* operator->() {
        return &buff;
    }

    constexpr operator bool() const {
        return true;
    }

private:
    command_buffer_t buff;
};

class with_render_pass {
public:
    with_render_pass(with_buffer& buffer, const vk::render_pass_begin_info& begin_info, vk::subpass_contents contents): buff(buffer) {
        buff->cmd_begin_render_pass(begin_info, contents);
    }

    ~with_render_pass() {
        buff->cmd_end_render_pass();
    }

    with_buffer& operator*() {
        return buff;
    }

    with_buffer* operator->() {
        return &buff;
    }

    constexpr operator bool() const {
        return true;
    }

private:
    with_buffer& buff;
};

class with_rendering {
public:
    with_rendering(with_buffer& buffer, const vk::rendering_info& info): buff(buffer) {
        buff->cmd_begin_rendering(info);
    }

    ~with_rendering() {
        buff->cmd_end_rendering();
    }

    with_buffer& operator*() {
        return buff;
    }

    with_buffer* operator->() {
        return &buff;
    }

    constexpr operator bool() const {
        return true;
    }

private:
    with_buffer& buff;
};
} // namespace vk
