add_executable(mouse_streamer ./mouse_streamer.cpp)
target_link_libraries(mouse_streamer PRIVATE glm::glm self::src)

add_executable(mouse_client ./mouse_client.cpp)
target_link_libraries(mouse_client PRIVATE glm::glm self::src X11 Xtst)

find_package(glfw3 REQUIRED)

add_executable(hello_triangle ./hello_triangle.cpp)
target_link_libraries(hello_triangle PRIVATE glm::glm self::src glfw vulkan)
add_dependencies(hello_triangle vulkan_api_target)

add_executable(hello_triangle_modernized ./hello_triangle_modernized.cpp)
target_link_libraries(hello_triangle_modernized PRIVATE glm::glm self::src glfw vulkan)
add_dependencies(hello_triangle_modernized vulkan_api_target)

add_executable(hello_triangle_index_buffer ./hello_triangle_index_buffer.cpp)
target_link_libraries(hello_triangle_index_buffer PRIVATE glm::glm self::src glfw vulkan)
add_dependencies(hello_triangle_index_buffer vulkan_api_target)

add_executable(hello_triangle_ubo ./hello_triangle_ubo.cpp)
target_link_libraries(hello_triangle_ubo PRIVATE glm::glm self::src glfw vulkan)
add_dependencies(hello_triangle_ubo vulkan_api_target)

add_executable(hello_triangle_textured ./hello_triangle_textured.cpp)
target_link_libraries(hello_triangle_textured PRIVATE glm::glm self::src glfw vulkan stb::stb)
add_dependencies(hello_triangle_textured vulkan_api_target)

add_executable(hello_triangle_depth_buffering ./hello_triangle_depth_buffering.cpp)
target_link_libraries(hello_triangle_depth_buffering PRIVATE glm::glm self::src glfw vulkan stb::stb)
add_dependencies(hello_triangle_depth_buffering vulkan_api_target)

find_package(tinyobjloader REQUIRED)

add_executable(vk_loading_models ./vk_loading_models.cpp)
target_link_libraries(vk_loading_models PRIVATE glm::glm self::src glfw vulkan stb::stb tinyobjloader::tinyobjloader)
add_dependencies(vk_loading_models vulkan_api_target)

add_executable(vk_perspective_camera vk_perspective_camera.cpp)
target_link_libraries(vk_perspective_camera glm::glm self::src glfw vulkan stb::stb tinyobjloader::tinyobjloader)
add_dependencies(vk_perspective_camera vulkan_api_target)

add_executable(vk_tacz_gun vk_tacz_gun.cpp)
target_link_libraries(vk_tacz_gun glm::glm self::src glfw vulkan stb::stb nlohmann::json)
add_dependencies(vk_tacz_gun vulkan_api_target)

add_executable(coro_test ./coro_test.cpp)
target_link_libraries(coro_test PRIVATE self::src uring)
add_dependencies(coro_test io_uring_cg)

add_executable(fs_watch ./fs_watch.cpp)
target_link_libraries(fs_watch PRIVATE self::src uring)
add_dependencies(fs_watch io_uring_cg)

add_executable(mcwe ./minecraft_world_edit.cpp)
target_link_libraries(mcwe PRIVATE glm::glm self::src z)
