add_library(simdutf_tests_helpers_random_utf8 STATIC
    random_utf8.h
    random_utf8.cpp)

add_library(simdutf::tests::helpers::random_utf8 ALIAS simdutf_tests_helpers_random_utf8)
