include(ExternalProject)

function(submodule_build _project_name)
    set(oneValueArgs CMAKE_LISTS_DIR)
    set(multiValueArgs CMAKE_ARGS)
    cmake_parse_arguments(${_project_name} "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    ExternalProject_Add(
        ${_project_name}
        DOWNLOAD_COMMAND ""
        SOURCE_DIR "${CMAKE_SOURCE_DIR}/submodules/${_project_name}"
        BINARY_DIR "${CMAKE_BINARY_DIR}/submodules/${_project_name}"
        INSTALL_DIR "${CMAKE_BINARY_DIR}/3rd"
        CMAKE_ARGS
            "${${_project_name}_CMAKE_ARGS}"
            -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/3rd
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    )
endfunction()
