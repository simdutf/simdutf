add_library(simdutf_benchmarks_benchmark STATIC
    benchmark_base.cpp
    benchmark_base.h
    cmdline.cpp
    cmdline.h
    benchmark.cpp
    benchmark.h
)
target_include_directories(simdutf_benchmarks_benchmark PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  # People who run macOS often use brew.
  list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/icu4c/include")
  list(APPEND CMAKE_LIBRARY_PATH "/opt/homebrew/opt/icu4c/lib")
endif()

# There are many components:  tu dt in io i18n data
find_package(ICU COMPONENTS uc)
if(ICU_FOUND)
  target_link_libraries(simdutf_benchmarks_benchmark PUBLIC  ICU::uc)
  target_compile_definitions(simdutf_benchmarks_benchmark PRIVATE ICU_AVAILABLE=1)
  #### We could use more components:
  #  target_link_libraries(simdutf_benchmarks_benchmark PUBLIC  ICU::i18n)
  #  target_link_libraries(simdutf_benchmarks_benchmark PUBLIC  ICU::in)
  #  target_link_libraries(simdutf_benchmarks_benchmark PUBLIC  ICU::tu)
  #  target_link_libraries(simdutf_benchmarks_benchmark PUBLIC  ICU::io)
  #  target_link_libraries(simdutf_benchmarks_benchmark PUBLIC  ICU::data)
else()
  message(STATUS "We rely on the system's ICU. It was not found!")
  if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    message(STATUS "You may be able to install it using 'brew'")
  endif()
endif(ICU_FOUND)

if(Iconv_FOUND)
  target_compile_definitions(simdutf_benchmarks_benchmark PRIVATE ICONV_AVAILABLE=1)
  if(NOT(Iconv_IS_BUILT_IN))
    target_link_libraries(simdutf_benchmarks_benchmark PUBLIC  ${Iconv_LIBRARY})
    target_include_directories(simdutf_benchmarks_benchmark PUBLIC ${Iconv_INCLUDE_DIR})
  endif(NOT(Iconv_IS_BUILT_IN))
endif(Iconv_FOUND)

set_property(TARGET simdutf_benchmarks_benchmark PROPERTY CXX_STANDARD 17)
set_property(TARGET simdutf_benchmarks_benchmark PROPERTY CXX_STANDARD_REQUIRED ON)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND NOT MINGW)
  target_link_libraries(simdutf_benchmarks_benchmark PUBLIC stdc++fs)
endif()

target_link_libraries(simdutf_benchmarks_benchmark PUBLIC simdutf simdutf::tests::helpers)

add_library(simdutf::benchmarks::benchmark ALIAS simdutf_benchmarks_benchmark)
