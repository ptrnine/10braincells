add_executable(tests-async async.cpp)

target_compile_options(tests-async PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-ctor-dtor-privacy>)
target_compile_options(tests-async PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-sign-conversion>)

target_link_libraries(tests-async self::src uring Catch2::Catch2 Catch2::Catch2WithMain)
catch_discover_tests(tests-async)
