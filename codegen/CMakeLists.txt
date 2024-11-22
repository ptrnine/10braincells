add_executable(flags flags.cpp)
target_link_libraries(flags PRIVATE self::src)

add_executable(cg_as_tuple as_tuple.cpp)
target_link_libraries(cg_as_tuple PRIVATE self::src)
