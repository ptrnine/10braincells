function(codegen _program_name)
    set(oneValueArgs)
    set(multiValueArgs ARGS OUTPUT)
    cmake_parse_arguments(${_program_name} "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    add_custom_command(
        OUTPUT
            ${${_program_name}_OUTPUT}
        COMMAND
            "${CMAKE_BINARY_DIR}/codegen/cg_${_program_name}"  # program name
            ${${_program_name}_ARGS}                           # additional args
        DEPENDS
            "${CMAKE_BINARY_DIR}/codegen/cg_${_program_name}"
        WORKING_DIRECTORY
            "${CMAKE_SOURCE_DIR}"
    )

    add_custom_target(${_program_name}_target DEPENDS ${${_program_name}_OUTPUT})
endfunction()
