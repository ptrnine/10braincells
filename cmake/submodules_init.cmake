macro(submodules_init)
    find_package(Git QUIET)
    if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
        message(STATUS "Submodules init...")
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                TIMEOUT 100000
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                RESULT_VARIABLE GIT_SUBMOD_RESULT)
        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init --recursive failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
        endif()
    else()
        message(FATAL_ERROR "Can't find git")
    endif()
endmacro()
