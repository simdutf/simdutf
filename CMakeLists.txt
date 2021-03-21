cmake_minimum_required(VERSION 3.13)

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
add_subdirectory(benchmarks)
