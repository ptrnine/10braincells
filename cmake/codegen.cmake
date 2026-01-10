function(codegen _target_name)
    set(oneValueArgs PROGRAM)
    set(multiValueArgs ARGS OUTPUT)
    cmake_parse_arguments(${_target_name} "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    add_custom_command(
        OUTPUT
            ${${_target_name}_OUTPUT}
        COMMAND
            "${CMAKE_BINARY_DIR}/codegen/${${_target_name}_PROGRAM}"   # program name
            ${${_target_name}_ARGS}                                    # additional args
        VERBATIM
        DEPENDS
            "${CMAKE_BINARY_DIR}/codegen/${${_target_name}_PROGRAM}"
        WORKING_DIRECTORY
            "${CMAKE_SOURCE_DIR}"
    )

    add_custom_target(${_target_name} DEPENDS ${${_target_name}_OUTPUT})
endfunction()
