add_library(simdutf_benchmarks_benchmark STATIC
    benchmark_base.cpp
    benchmark_base.h
    cmdline.cpp
    cmdline.h
    benchmark.cpp
    benchmark.h
)
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND NOT MINGW)
  target_link_libraries(simdutf_benchmarks_benchmark PUBLIC stdc++fs)
endif()
target_link_libraries(simdutf_benchmarks_benchmark PUBLIC simdutf simdutf::tests::helpers)
add_library(simdutf::benchmarks::benchmark ALIAS simdutf_benchmarks_benchmark)
