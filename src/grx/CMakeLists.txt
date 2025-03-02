file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/src/grx/vk")

codegen(vulkan_api
    ARGS "api=${VULKAN_API}" "dir=${CMAKE_BINARY_DIR}/src/grx/vk"
    OUTPUT
        "${CMAKE_BINARY_DIR}/src/grx/vk/commands.hpp"
        "${CMAKE_BINARY_DIR}/src/grx/vk/structs.hpp"
        "${CMAKE_BINARY_DIR}/src/grx/vk/flags.hpp"
        "${CMAKE_BINARY_DIR}/src/grx/vk/enums.hpp"
        "${CMAKE_BINARY_DIR}/src/grx/vk/constants.hpp"
        "${CMAKE_BINARY_DIR}/src/grx/vk/types.hpp"
        "${CMAKE_BINARY_DIR}/src/grx/vk/function_types.hpp"
        "${CMAKE_BINARY_DIR}/src/grx/vk/physical_device.hpp"
)
