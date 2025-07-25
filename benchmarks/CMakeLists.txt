if (CMAKE_COMPILER_IS_GNUCXX)
  add_compile_options("-fdiagnostics-color")
  add_compile_options("-Wpedantic")
  add_compile_options("-Wextra")
  add_compile_options("-Wall")
endif()


if(MSVC)
  add_compile_options("/Zi" "/EHsc" "/GR")
endif()

if(SIMDUTF_SANITIZE)
  add_compile_options(-fsanitize=address  -fno-omit-frame-pointer -fno-sanitize-recover=all)
  add_compile_definitions(ASAN_OPTIONS=detect_leaks=1)
endif()


add_executable(stream stream.cpp)
target_link_libraries(stream PUBLIC simdutf)

if(SIMDUTF_ICONV)
find_package(Iconv)
if(Iconv_FOUND)
  target_compile_definitions(stream PRIVATE ICONV_AVAILABLE=1)
  if(Iconv_IS_BUILT_IN)
    message(STATUS "Iconv is part of the C library.")
  else(Iconv_IS_BUILT_IN)
    message(STATUS "Iconv is a separate library, headers at ${Iconv_INCLUDE_DIR}, binary at ${Iconv_LIBRARY}")
    target_link_libraries(stream PUBLIC  ${Iconv_LIBRARY})
    target_include_directories(stream PUBLIC ${Iconv_INCLUDE_DIR})
  endif(Iconv_IS_BUILT_IN)
else(Iconv_FOUND)
  message(STATUS "Iconv was not found!")
endif(Iconv_FOUND)
else(SIMDUTF_ICONV)
  message(STATUS "We are not seeking iconv.")
endif(SIMDUTF_ICONV)

add_subdirectory(src)

add_executable(benchmark benchmark.cpp)
target_link_libraries(benchmark PUBLIC simdutf::benchmarks::benchmark)

set_property(TARGET benchmark PROPERTY CXX_STANDARD 17)
set_property(TARGET benchmark PROPERTY CXX_STANDARD_REQUIRED ON)

if(WIN32 AND BUILD_SHARED_LIBS)
  # Copy the simdutf dll into the benchmark directory
  add_custom_command(TARGET stream POST_BUILD        # Adds a post-build event
    COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake -E copy_if_different..."
        "$<TARGET_FILE:simdutf>"      # <--this is in-file
        "$<TARGET_FILE_DIR:random_fuzzer>")                 # <--this is out-file path
endif()


set_property(TARGET stream PROPERTY CXX_STANDARD 17)
set_property(TARGET stream PROPERTY CXX_STANDARD_REQUIRED ON)

add_executable(alignment alignment.cpp)
target_link_libraries(alignment PUBLIC simdutf)

set_property(TARGET alignment PROPERTY CXX_STANDARD 17)
set_property(TARGET alignment PROPERTY CXX_STANDARD_REQUIRED ON)



find_package(Threads)

if(Threads_FOUND)
  add_executable(threaded threaded.cpp)
  target_link_libraries(threaded PUBLIC simdutf)
  target_link_libraries(threaded PUBLIC Threads::Threads)

  set_property(TARGET threaded PROPERTY CXX_STANDARD 17)
  set_property(TARGET threaded PROPERTY CXX_STANDARD_REQUIRED ON)
endif(Threads_FOUND)

option(SIMDUTF_BENCHMARK_BASE64 "Whether the base64 benchmarks are included as part of the CMake Build (requires C++17 or better)." ON)

if(SIMDUTF_BENCHMARK_BASE64)
  if(CMAKE_CXX_COMPILER_ID STREQUAL Clang AND "x${CMAKE_CXX_SIMULATE_ID}" STREQUAL "xMSVC")
    message(STATUS "Not building base64 benchmarks when using clang-cl due to build errors with the aklomp/base64 dependency.")
  else()
    add_subdirectory(base64)
  endif()
endif(SIMDUTF_BENCHMARK_BASE64)


add_executable(benchmark_to_well_formed_utf16 benchmark_to_well_formed_utf16.cpp)
target_link_libraries(benchmark_to_well_formed_utf16 PUBLIC simdutf)
target_link_libraries(benchmark_to_well_formed_utf16 PUBLIC simdutf::benchmarks::benchmark)
set_property(TARGET benchmark_to_well_formed_utf16 PROPERTY CXX_STANDARD 17)
set_property(TARGET benchmark_to_well_formed_utf16 PROPERTY CXX_STANDARD_REQUIRED ON)
