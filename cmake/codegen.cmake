function(codegen _program_name)
    set(oneValueArgs OUTPUT)
    set(multiValueArgs ARGS)
    cmake_parse_arguments(${_program_name} "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    add_custom_command(
        OUTPUT
            "${CMAKE_BINARY_DIR}/${${_program_name}_OUTPUT}"
        COMMAND
            "${CMAKE_BINARY_DIR}/codegen/cg_${_program_name}"   # program name
            "${${_program_name}_ARGS}"                          # additional args
            "${CMAKE_BINARY_DIR}/${${_program_name}_OUTPUT}"    # output file
        DEPENDS
            "${CMAKE_BINARY_DIR}/codegen/cg_${_program_name}"
        WORKING_DIRECTORY
            "${CMAKE_SOURCE_DIR}"
    )

    add_custom_target(${_program_name}_target DEPENDS "${CMAKE_BINARY_DIR}/${${_program_name}_OUTPUT}")
endfunction()
