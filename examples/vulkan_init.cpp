#include <grx/vk/physical_device.hpp>
#include <grx/vk/extensions.hpp>
#include <util/log.hpp>

int main() {
    vk::vk_lib lib{"/usr/lib64/libvulkan.so"};

    auto instance = lib.create_instance(vk::instance_create_info::pass(
        vk::application_info{
            .application_name    = "Test App",
            .application_version = {1, 0, 0},
            .engine_name         = "Test Engine",
            .engine_version      = {1, 0, 0},
            .api_version         = {1, 0, 0},
        },
        vk::enabled_extensions{"VK_KHR_display"}));

    auto devs = instance.physical_devices();
    for (auto&& dev : devs.value()) {
        auto props = dev.properties();
        util::glog().warn("Device: {} {} {}", props.device_name, props.api_version, props.driver_version);
    }
}
