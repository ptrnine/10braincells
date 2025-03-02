add_executable(mouse_streamer ./mouse_streamer.cpp)
target_link_libraries(mouse_streamer PRIVATE glm::glm self::src)

add_executable(mouse_client ./mouse_client.cpp)
target_link_libraries(mouse_client PRIVATE glm::glm self::src X11 Xtst)
