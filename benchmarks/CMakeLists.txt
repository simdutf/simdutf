if (CMAKE_COMPILER_IS_GNUCXX) 
    add_compile_options("-fdiagnostics-color")
endif()

add_compile_options("-Wpedantic")
add_compile_options("-Wextra")
add_compile_options("-Wall")
# add_compile_options("-Werror")

add_subdirectory(src)

add_executable(benchmark benchmark.cpp)
target_link_libraries(benchmark PUBLIC simdutf simdutf::benchmarks::benchmark)
