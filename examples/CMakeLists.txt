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

add_executable(coro_test ./coro_test.cpp)
target_link_libraries(coro_test PRIVATE self::src uring)
add_dependencies(coro_test io_uring_cg)
