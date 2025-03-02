add_executable(mouse_streamer ./mouse_streamer.cpp)
target_link_libraries(mouse_streamer PRIVATE glm::glm self::src)

add_executable(mouse_client ./mouse_client.cpp)
target_link_libraries(mouse_client PRIVATE glm::glm self::src X11 Xtst)

add_executable(vulkan_init ./vulkan_init.cpp)
target_link_libraries(vulkan_init PRIVATE glm::glm self::src)
add_dependencies(vulkan_init vulkan_api_target)
