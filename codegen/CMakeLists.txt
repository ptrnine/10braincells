add_executable(cg_flags flags.cpp)
target_link_libraries(cg_flags PRIVATE self::src)

add_executable(cg_as_tuple as_tuple.cpp)
# Cannot use self::src here because self::src depends on generated code
target_include_directories(cg_as_tuple SYSTEM PRIVATE ${self_src_system_includes})
target_include_directories(cg_as_tuple PRIVATE ${self_src_includes})
