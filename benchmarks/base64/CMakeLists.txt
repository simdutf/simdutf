include(../../cmake/CPM.cmake)
CPMAddPackage(
  NAME base64
  GITHUB_REPOSITORY aklomp/base64
  VERSION 0.5.2
  OPTIONS "BASE64_BUILD_CLI Off"
)


add_executable(benchmark_base64 benchmark_base64.cpp)

target_link_libraries(benchmark_base64 PUBLIC simdutf)
target_link_libraries(benchmark_base64 PUBLIC base64)
target_link_libraries(benchmark_base64 PUBLIC simdutf::benchmarks::benchmark)
