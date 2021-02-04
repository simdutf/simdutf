include(${PROJECT_SOURCE_DIR}/cmake/add_cpp_test.cmake)
link_libraries(simdutf)
add_cpp_test(unicode_tests)
target_link_libraries(unicode_tests PUBLIC simdutf-headers)