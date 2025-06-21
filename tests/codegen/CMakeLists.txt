add_executable(codegen-vk
    member_functions.cpp
)

target_compile_options(tests-core PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-ctor-dtor-privacy>)
target_compile_options(tests-core PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-sign-conversion>)
#target_compile_options(tests-core PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fsanitize=undefined>)
#target_link_options(tests-core PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fsanitize=undefined>)

find_package(pugixml REQUIRED)
target_link_libraries(codegen-vk self::src Catch2::Catch2 Catch2::Catch2WithMain pugixml::pugixml)
catch_discover_tests(codegen-vk)
