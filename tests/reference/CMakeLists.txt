add_library(simdutf_tests_helpers_validate_utf8 STATIC
    validate_utf8.h
    validate_utf8.cpp)
add_library(simdutf::tests::helpers::validate_utf8 ALIAS simdutf_tests_helpers_validate_utf8)
target_link_libraries(simdutf_tests_helpers_validate_utf8 PUBLIC simdutf-headers)
