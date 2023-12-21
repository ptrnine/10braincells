add_executable(tests-nostd
    concepts.cpp
    int_const.cpp
    meta.cpp
    traits.cpp
    utility.cpp
)

target_compile_options(tests-nostd PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-ctor-dtor-privacy>)
target_compile_definitions(tests-nostd PRIVATE -DENABLE_FORWARD_MACRO)

target_link_libraries(tests-nostd self::src Catch2::Catch2WithMain)
catch_discover_tests(tests-nostd)
