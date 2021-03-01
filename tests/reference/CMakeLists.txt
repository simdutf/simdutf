add_library(simdutf_tests_reference STATIC
  encode_utf8.h
  encode_utf8.cpp
  encode_utf16.h
  encode_utf16.cpp
  validate_utf8.h
  validate_utf8.cpp
  validate_utf16.h
  validate_utf16.cpp)
add_library(simdutf::tests::reference ALIAS simdutf_tests_reference)
target_link_libraries(simdutf_tests_reference PUBLIC simdutf)
