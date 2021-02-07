add_library(simdutf_benchmarks_benchmark STATIC
    benchmark_base.cpp
    benchmark_base.h
    cmdline.cpp
    cmdline.h
    benchmark.cpp
    benchmark.h
)

target_link_libraries(simdutf_benchmarks_benchmark PUBLIC simdutf simdutf::tests::helpers::random_utf8)
add_library(simdutf::benchmarks::benchmark ALIAS simdutf_benchmarks_benchmark)
