add_executable(tests-core
    concepts.cpp
    int_const.cpp
    meta.cpp
    traits.cpp
    tuple.cpp
    utility.cpp
    var.cpp
    opt.cpp
    ranges.cpp
    function.cpp
    ca_traits.cpp
    ptr_map.cpp
    generator.cpp
    io.cpp
    args.cpp
    loophole.cpp
    byteconv.cpp
)

target_compile_options(tests-core PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-ctor-dtor-privacy>)
target_compile_options(tests-core PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-sign-conversion>)
#target_compile_options(tests-core PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fsanitize=undefined>)
#target_link_options(tests-core PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fsanitize=undefined>)

target_link_libraries(tests-core self::src Catch2::Catch2 Catch2::Catch2WithMain)
catch_discover_tests(tests-core)
