add_subdirectory(helpers)
add_subdirectory(reference)

include(${PROJECT_SOURCE_DIR}/cmake/add_cpp_test.cmake)
link_libraries(simdutf)
add_cpp_test(unicode_tests)
target_link_libraries(unicode_tests
    PUBLIC
        simdutf-headers simdutf::tests::helpers::random_utf8
        simdutf-headers simdutf::tests::helpers::validate_utf8
)
