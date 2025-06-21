add_executable(mouse_streamer ./mouse_streamer.cpp)
target_link_libraries(mouse_streamer PRIVATE glm::glm self::src)

add_executable(mouse_client ./mouse_client.cpp)
target_link_libraries(mouse_client PRIVATE glm::glm self::src X11 Xtst)

add_executable(hello_triangle ./hello_triangle.cpp)
target_link_libraries(hello_triangle PRIVATE glm::glm self::src glfw vulkan)
add_dependencies(hello_triangle vulkan_api_target)
