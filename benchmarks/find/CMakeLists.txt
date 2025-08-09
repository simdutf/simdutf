add_executable(findbenchmark findbenchmark.cpp)
set_property(TARGET findbenchmark PROPERTY CXX_STANDARD 20)
set_property(TARGET findbenchmark PROPERTY CXX_STANDARD_REQUIRED ON)

target_link_libraries(findbenchmark PUBLIC simdutf::benchmarks::benchmark)

