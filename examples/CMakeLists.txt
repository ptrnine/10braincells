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

add_executable(uring_example ./uring.cpp)
target_link_libraries(uring_example PRIVATE self::src uring)
add_dependencies(uring_example io_uring_cg)
