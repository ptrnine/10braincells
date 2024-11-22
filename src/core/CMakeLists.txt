codegen(as_tuple ARGS "max_fields=16" OUTPUT "src/core/as_tuple.cg.hpp")

add_library(tbc_core INTERFACE)
add_dependencies(tbc_core as_tuple_target)
