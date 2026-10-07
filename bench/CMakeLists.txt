# Benchmarks (self-contained, no tests framework)
#
# bench_hashes compares core::xxhash (src/core/xxhash.hpp) against the
# upstream xxHash reference implementation from the sibling repo ../xxhash.
add_executable(bench_hashes
    hashes.cpp
    ${CMAKE_SOURCE_DIR}/../xxhash/xxhash.c
)
target_include_directories(bench_hashes PRIVATE ${CMAKE_SOURCE_DIR}/../xxhash)
target_link_libraries(bench_hashes self::src)
target_compile_options(bench_hashes PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-unused-result>)
target_compile_options(bench_hashes PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-sign-conversion>)

# bench_hash_map compares core::hash_map (src/core/robin_map.hpp) against
# std::unordered_map: fill (clear + insert), successful lookups, misses.
add_executable(bench_hash_map
    hash_map.cpp
)
target_link_libraries(bench_hash_map self::src)
target_compile_options(bench_hash_map PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-unused-result>)
