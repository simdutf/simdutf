cmake_minimum_required(VERSION 3.15)

project(simdutf
  DESCRIPTION "Fast UTF tools"
  LANGUAGES CXX 
)

set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 1)
set(PROJECT_VERSION_PATCH 0)
include(GNUInstallDirs)
include(cmake/simdutf-flags.cmake)

set(SIMDUTF_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

enable_testing()
add_subdirectory(include)
add_subdirectory(src)
add_subdirectory(tests)


if(CMAKE_CXX_COMPILER_ID MATCHES GNU AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 8.0)
  message(STATUS "The benchmark tool requires GCC 8.0 or better.")
else()
  add_subdirectory(benchmarks)
endif()


