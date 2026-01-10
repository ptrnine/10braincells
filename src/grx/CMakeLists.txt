file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/src/grx/vk")

codegen(vulkan_api_target
    PROGRAM cg_vulkan_api
    ARGS "api=${VULKAN_API}" "dir=${CMAKE_BINARY_DIR}/src/grx/vk"
    OUTPUT
        "${CMAKE_BINARY_DIR}/src/grx/vk/commands.cg.hpp"
        "${CMAKE_BINARY_DIR}/src/grx/vk/structs.cg.hpp"
        "${CMAKE_BINARY_DIR}/src/grx/vk/flags.cg.hpp"
        "${CMAKE_BINARY_DIR}/src/grx/vk/enums.cg.hpp"
        "${CMAKE_BINARY_DIR}/src/grx/vk/constants.cg.hpp"
        "${CMAKE_BINARY_DIR}/src/grx/vk/types.cg.hpp"
        "${CMAKE_BINARY_DIR}/src/grx/vk/function_types.cg.hpp"
        "${CMAKE_BINARY_DIR}/src/grx/vk/physical_device.cg.hpp"
        "${CMAKE_BINARY_DIR}/src/grx/vk/device.cg.hpp"
        "${CMAKE_BINARY_DIR}/src/grx/vk/swapchain.cg.hpp"
        "${CMAKE_BINARY_DIR}/src/grx/vk/image_view.cg.hpp"
        "${CMAKE_BINARY_DIR}/src/grx/vk/shader_module.cg.hpp"
        "${CMAKE_BINARY_DIR}/src/grx/vk/pipeline_layout.cg.hpp"
        "${CMAKE_BINARY_DIR}/src/grx/vk/pipeline.cg.hpp"
        "${CMAKE_BINARY_DIR}/src/grx/vk/render_pass.cg.hpp"
        "${CMAKE_BINARY_DIR}/src/grx/vk/framebuffer.cg.hpp"
        "${CMAKE_BINARY_DIR}/src/grx/vk/command_pool.cg.hpp"
        "${CMAKE_BINARY_DIR}/src/grx/vk/command_buffer.cg.hpp"
        "${CMAKE_BINARY_DIR}/src/grx/vk/semaphore.cg.hpp"
        "${CMAKE_BINARY_DIR}/src/grx/vk/fence.cg.hpp"
        "${CMAKE_BINARY_DIR}/src/grx/vk/queue.cg.hpp"
)
