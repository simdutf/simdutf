set(SIMDUTF_TEST_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../)

add_subdirectory(helpers)
add_subdirectory(reference)

include(${PROJECT_SOURCE_DIR}/cmake/add_cpp_test.cmake)
link_libraries(simdutf)

add_cpp_test(validate_utf8_basic_tests)
target_link_libraries(validate_utf8_basic_tests PUBLIC simdutf-headers)

add_cpp_test(validate_utf8_brute_force_tests)
target_link_libraries(validate_utf8_brute_force_tests
    PUBLIC
        simdutf-headers
        simdutf::tests::helpers::random_utf8
        simdutf::tests::helpers::validate_utf8
)

add_cpp_test(validate_utf8_puzzler_tests)
target_link_libraries(validate_utf8_puzzler_tests PUBLIC simdutf-headers)

add_cpp_test(validate_utf16_basic_tests)
target_link_libraries(validate_utf16_basic_tests PUBLIC simdutf-headers)
target_compile_options(validate_utf16_basic_tests PRIVATE -Wfatal-errors)
