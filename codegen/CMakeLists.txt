add_executable(cg_as_tuple as_tuple.cpp)
# Cannot use self::src here because self::src depends on generated code
target_include_directories(cg_as_tuple SYSTEM PRIVATE ${self_src_system_includes})
target_include_directories(cg_as_tuple PRIVATE ${self_src_includes})

# Run codegen required by self::src
codegen(as_tuple_target
    PROGRAM cg_as_tuple
    ARGS "max_fields=16" "out=${CMAKE_BINARY_DIR}/src/core/as_tuple.cg.hpp"
    OUTPUT "${CMAKE_BINARY_DIR}/src/core/as_tuple.cg.hpp"
)

add_executable(cg_flags flags.cpp)
# Cannot use self::src here because self::src depends on generated code
target_include_directories(cg_flags SYSTEM PRIVATE ${self_src_system_includes})
target_include_directories(cg_flags PRIVATE ${self_src_includes})
add_dependencies(cg_flags as_tuple_target)

find_package(pugixml REQUIRED)
add_executable(cg_vulkan_api vulkan.cpp)
target_link_libraries(cg_vulkan_api PRIVATE self::src pugixml::pugixml)
