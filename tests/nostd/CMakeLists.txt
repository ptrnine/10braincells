add_executable(tests-nostd
    concepts.cpp
    int_const.cpp
    meta.cpp
    traits.cpp
    tuple.cpp
    utility.cpp
)

target_compile_options(tests-nostd PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-ctor-dtor-privacy>)
target_compile_options(tests-nostd PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-sign-conversion>)

target_link_libraries(tests-nostd self::src Catch2::Catch2 Catch2::Catch2WithMain)
catch_discover_tests(tests-nostd)
