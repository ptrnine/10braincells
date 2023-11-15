cmake_minimum_required(VERSION 3.16.3)
project(tbc)

include(cmake/submodules_init.cmake)
include(cmake/submodule_build.cmake)

include_directories(SYSTEM "${CMAKE_BINARY_DIR}/3rd/include")
include_directories(${CMAKE_SOURCE_DIR}/src)
link_directories("${CMAKE_BINARY_DIR}/3rd/lib")
link_directories("${CMAKE_BINARY_DIR}/3rd/lib64")
set(CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}/3rd/lib/cmake")

submodules_init()
submodule_build(bullet3 CMAKE_ARGS -DBUILD_UNIT_TESTS=OFF -DBUILD_BULLET2_DEMOS=OFF)
submodule_build(assimp CMAKE_ARGS -DBUILD_SHARED_LIBS=OFF -DASSIMP_BUILD_TESTS=OFF -DASSIMP_WARNINGS_AS_ERRORS=OFF)
submodule_build(Catch2 CMAKE_ARGS -DCATCH_INSTALL_DOCS=OFF -DCATCH_BUILD_TESTING=OFF)
submodule_build(glfw CMAKE_ARGS -DGLFW_BUILD_EXAMPLES=OFF -DGLFW_BUILD_DOCS=OFF -DGLFW_BUILD_TESTS=OFF)
include_directories(SYSTEM submodules/glm)

set(CMAKE_CXX_STANDARD 23)

add_executable(test ./test.cpp)
