set(SIMDUTF_TEST_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../)
if(MSVC)
  add_compile_options("/Zi" "/EHsc" "/GR")
endif()

add_subdirectory(helpers)
add_subdirectory(reference)

include(${PROJECT_SOURCE_DIR}/cmake/add_cpp_test.cmake)
link_libraries(simdutf)

add_cpp_test(validate_utf8_basic_tests)
target_link_libraries(validate_utf8_basic_tests PUBLIC simdutf-headers)

add_cpp_test(validate_utf8_brute_force_tests)
target_link_libraries(validate_utf8_brute_force_tests
  PUBLIC simdutf-headers
         simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(validate_utf8_puzzler_tests)
target_link_libraries(validate_utf8_puzzler_tests PUBLIC simdutf-headers)

add_cpp_test(validate_utf16_basic_tests)
target_link_libraries(validate_utf16_basic_tests
  PUBLIC simdutf
         simdutf::tests::reference
         simdutf::tests::helpers)

find_package(Python3 COMPONENTS Interpreter)
if(Python3_FOUND)
  set(SCRIPT ${SIMDUTF_SOURCE_DIR}/scripts/sse_validate_utf16le_testcases.py)
  add_custom_command(TARGET validate_utf16_basic_tests
                   COMMAND ${Python3_EXECUTABLE} ${SCRIPT}
                   DEPENDS ${SCRIPT}
                   VERBATIM
  )
  unset(SCRIPT)
endif(Python3_FOUND)

add_cpp_test(convert_valid_utf8_to_utf16_tests)
target_link_libraries(convert_valid_utf8_to_utf16_tests
  PUBLIC simdutf
         simdutf::tests::reference
         simdutf::tests::helpers)

add_cpp_test(convert_utf8_to_utf16_tests)
target_link_libraries(convert_utf8_to_utf16_tests
  PUBLIC simdutf
         simdutf::tests::reference
         simdutf::tests::helpers)

add_cpp_test(convert_utf16_to_utf8_tests)
target_link_libraries(convert_utf16_to_utf8_tests
  PUBLIC simdutf
         simdutf::tests::reference
         simdutf::tests::helpers)

add_cpp_test(convert_valid_utf16_to_utf8_tests)
target_link_libraries(convert_valid_utf16_to_utf8_tests
  PUBLIC simdutf
         simdutf::tests::reference
         simdutf::tests::helpers)

add_cpp_test(count_utf8)
target_link_libraries(count_utf8
  PUBLIC simdutf
         simdutf::tests::reference
         simdutf::tests::helpers)

add_cpp_test(count_utf16)
target_link_libraries(count_utf16
  PUBLIC simdutf
         simdutf::tests::reference
         simdutf::tests::helpers)

add_cpp_test(basic_fuzzer)
target_link_libraries(basic_fuzzer
  PUBLIC simdutf
         simdutf::tests::reference
         simdutf::tests::helpers)