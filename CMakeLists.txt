cmake_minimum_required(VERSION 3.16.3)
project(tbc)

set(CMAKE_CXX_STANDARD 26)

option(ENABLE_TESTS "Enable tests" ON)
option(ENABLE_ASAN "Enable address sanitizer" OFF)
option(BUILD_3RD "Build 3rd party libraries only" OFF)

if(BUILD_3RD)
    include(cmake/submodules_init.cmake)
    include(cmake/submodule_build.cmake)

    submodules_init()
    submodule_build(bullet3 CMAKE_ARGS -DBUILD_UNIT_TESTS=OFF -DBUILD_BULLET2_DEMOS=OFF)
    submodule_build(assimp CMAKE_ARGS -DBUILD_SHARED_LIBS=OFF -DASSIMP_BUILD_TESTS=OFF -DASSIMP_WARNINGS_AS_ERRORS=OFF)
    submodule_build(glfw CMAKE_ARGS -DGLFW_BUILD_EXAMPLES=OFF -DGLFW_BUILD_DOCS=OFF -DGLFW_BUILD_TESTS=OFF)

    if(ENABLE_TESTS)
        submodule_build(Catch2 CMAKE_ARGS
            -DCATCH_INSTALL_DOCS=OFF
            -DCATCH_BUILD_TESTING=OFF
        )
    endif()

    return()
endif()

list(APPEND CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}/3rd")

# Determine arch and add suitable include path
execute_process(COMMAND cmake/current_arch.sh "${CMAKE_C_COMPILER}"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE ARCH_INTERNAL)
set(ARCH "${ARCH_INTERNAL}" CACHE STRING "The target architecture")
message("Target architecture: ${ARCH}")

add_library(self::src INTERFACE IMPORTED)
set(self_src_includes "${CMAKE_SOURCE_DIR}/src" "${CMAKE_SOURCE_DIR}/src/arch/${ARCH}")
set_target_properties(self::src PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${self_src_includes}"
)

add_library(glm::glm INTERFACE IMPORTED)
set_target_properties(glm::glm PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/submodules/glm"
)

add_library(stb::stb INTERFACE IMPORTED)
set_target_properties(stb::stb PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/submodules/stb"
)

set(_cxx_flags
    -fno-strict-aliasing
    -Wall
    -Wextra
    -Wpedantic
    -Wcast-align
    -Wconversion
    -Wctor-dtor-privacy
    -Wextra-semi
    -Wfloat-equal
    -Wnon-virtual-dtor
    -Wold-style-cast
    -Woverloaded-virtual
    -Wredundant-decls
    -Wsign-conversion
    -Wsign-promo
    -Werror=unused-result
    -Wno-unused-function
    -Wno-missing-braces
    -Wno-redundant-decls
)

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
    list(APPEND _cxx_flags
        -fconcepts-diagnostics-depth=100
        -Wno-non-template-friend
    )
endif()

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    list(APPEND _cxx_flags
        -Wno-gnu-string-literal-operator-template
    )
endif()

if (ENABLE_ASAN)
    list(APPEND _cxx_flags -fsanitize=address)
    add_link_options($<$<COMPILE_LANGUAGE:CXX>:-fsanitize=address>)
endif()

foreach(_flag ${_cxx_flags})
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:${_flag}>)
endforeach()

add_subdirectory(codegen)

if(ENABLE_TESTS)
    add_subdirectory(tests)
endif()

# TODO: remove this later
add_executable(test2 test2.cpp)
target_link_libraries(test2 PRIVATE glm::glm stb::stb self::src GLEW GL glfw)
