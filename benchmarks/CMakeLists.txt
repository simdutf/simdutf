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

add_subdirectory(src)

add_executable(benchmark benchmark.cpp)
target_link_libraries(benchmark PUBLIC simdutf simdutf::benchmarks::benchmark)

set_property(TARGET benchmark PROPERTY CXX_STANDARD 17)
set_property(TARGET benchmark PROPERTY CXX_STANDARD_REQUIRED ON)

add_executable(stream stream.cpp)
target_link_libraries(stream PUBLIC simdutf)

set_property(TARGET stream PROPERTY CXX_STANDARD 17)
set_property(TARGET stream PROPERTY CXX_STANDARD_REQUIRED ON)
