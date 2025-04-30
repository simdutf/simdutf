include(../../cmake/CPM.cmake)
CPMAddPackage(
  NAME base64
  GITHUB_REPOSITORY aklomp/base64
  VERSION 0.5.2
  OPTIONS "BASE64_BUILD_CLI Off"
)

# Check if the compiler supports C++20
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("/std:c++20" MSVC_CXX20_SUPPORTED) # For MSVC
check_cxx_compiler_flag("-std=c++20" UNIX_CXX20_SUPPORTED) # For GCC/Clang

if((CMAKE_CXX_COMPILER_ID MATCHES "MSVC" AND MSVC_CXX20_SUPPORTED) OR
   (NOT CMAKE_CXX_COMPILER_ID MATCHES "MSVC" AND UNIX_CXX20_SUPPORTED))
    message(STATUS "Compiler supports C++20")
  add_executable(benchmark_base64 benchmark_base64.cpp)
  set_property(TARGET benchmark_base64 PROPERTY CXX_STANDARD 20)
  set_property(TARGET benchmark_base64 PROPERTY CXX_STANDARD_REQUIRED ON)

  target_link_libraries(benchmark_base64 PUBLIC base64)
  target_link_libraries(benchmark_base64 PUBLIC simdutf::benchmarks::benchmark)

  # check if simdutf was built with c++ 20 - otherwise atomic base64 operations are not available
  get_property(SIMDUTF_COMPILED_CXX_VERSION TARGET simdutf PROPERTY CXX_STANDARD)
  message(VERBOSE "simdutf version is ${SIMDUTF_COMPILED_CXX_VERSION}")
  target_compile_definitions(benchmark_base64 PRIVATE SIMDUTF_COMPILED_CXX_VERSION=${SIMDUTF_COMPILED_CXX_VERSION})
else()
  message(STATUS "Compiler does not support C++20. We not building the tool benchmark_base64.")
endif()
