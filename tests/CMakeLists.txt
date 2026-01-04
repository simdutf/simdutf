include(CTest)

set(SIMDUTF_TEST_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../)
if(MSVC)
  add_compile_options("/Zi" "/EHsc" "/GR" "/source-charset:utf-8")
endif()
add_subdirectory(helpers)
add_subdirectory(reference)

include(${PROJECT_SOURCE_DIR}/cmake/add_cpp_test.cmake)

add_cpp_test(null_safety_tests)
target_link_libraries(null_safety_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(readme_tests)
target_link_libraries(readme_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(random_fuzzer)
target_link_libraries(random_fuzzer
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)
if(SIMDUTF_FAST_TESTS)
 target_compile_definitions(random_fuzzer PRIVATE SIMDUTF_TEST_FUZZER_TRIALS=10)
else()
 target_compile_definitions(random_fuzzer PRIVATE SIMDUTF_TEST_FUZZER_TRIALS=10000)
endif()
if(WIN32 AND BUILD_SHARED_LIBS)
  # Copy the simdutf dll into the tests directory
  add_custom_command(TARGET random_fuzzer POST_BUILD        # Adds a post-build event
    COMMAND ${CMAKE_COMMAND} -E copy_if_different  # which executes "cmake -E copy_if_different..."
        "$<TARGET_FILE:simdutf>"      # <--this is in-file
        "$<TARGET_FILE_DIR:random_fuzzer>")                 # <--this is out-file path
endif()
add_cpp_test(special_tests)
target_link_libraries(special_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(validate_ascii_basic_tests)
target_link_libraries(validate_ascii_basic_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(validate_ascii_with_errors_tests)
target_link_libraries(validate_ascii_with_errors_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(bele_tests)
target_link_libraries(bele_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(validate_utf8_basic_tests)
target_link_libraries(validate_utf8_basic_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)


add_cpp_test(select_implementation)
target_link_libraries(select_implementation
  PUBLIC simdutf::tests::helpers
        simdutf::tests::reference)

add_cpp_test(validate_utf8_brute_force_tests)
target_link_libraries(validate_utf8_brute_force_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(validate_utf8_puzzler_tests)
target_link_libraries(validate_utf8_puzzler_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(validate_utf8_with_errors_tests)
target_link_libraries(validate_utf8_with_errors_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(validate_utf16le_basic_tests)
target_link_libraries(validate_utf16le_basic_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(validate_utf16be_basic_tests)
target_link_libraries(validate_utf16be_basic_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(validate_utf16le_with_errors_tests)
target_link_libraries(validate_utf16le_with_errors_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(validate_utf16be_with_errors_tests)
target_link_libraries(validate_utf16be_with_errors_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(validate_utf32_basic_tests)
target_link_libraries(validate_utf32_basic_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(validate_utf32_with_errors_tests)
target_link_libraries(validate_utf32_with_errors_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

find_package(Python3 COMPONENTS Interpreter)
if(Python3_FOUND)
  set(SCRIPT ${SIMDUTF_SOURCE_DIR}/scripts/sse_validate_utf16le_testcases.py)
  add_custom_command(TARGET validate_utf16le_basic_tests
                   COMMAND ${Python3_EXECUTABLE} ${SCRIPT}
                   POST_BUILD
                   VERBATIM
  )
  unset(SCRIPT)
endif(Python3_FOUND)
add_cpp_test(find_tests)

target_link_libraries(find_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(convert_latin1_to_utf8_tests)
target_link_libraries(convert_latin1_to_utf8_tests 
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(convert_latin1_to_utf16le_tests)
target_link_libraries(convert_latin1_to_utf16le_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(convert_latin1_to_utf16be_tests)
target_link_libraries(convert_latin1_to_utf16be_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(convert_latin1_to_utf32_tests)
target_link_libraries(convert_latin1_to_utf32_tests 
    PUBLIC simdutf::tests::helpers
      simdutf::tests::reference)

add_cpp_test(convert_utf8_to_latin1_tests)
target_link_libraries(convert_utf8_to_latin1_tests 
  PUBLIC simdutf::tests::helpers
        simdutf::tests::reference)

add_cpp_test(convert_utf8_to_latin1_with_errors_tests)
target_link_libraries(convert_utf8_to_latin1_with_errors_tests 
  PUBLIC simdutf::tests::helpers
          simdutf::tests::reference)
        
add_cpp_test(convert_valid_utf8_to_latin1_tests)
target_link_libraries(convert_valid_utf8_to_latin1_tests 
  PUBLIC simdutf::tests::helpers
        simdutf::tests::reference)
         
add_cpp_test(convert_valid_utf8_to_utf16le_tests)
target_link_libraries(convert_valid_utf8_to_utf16le_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(convert_valid_utf8_to_utf16be_tests)
target_link_libraries(convert_valid_utf8_to_utf16be_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(convert_valid_utf8_to_utf32_tests)
target_link_libraries(convert_valid_utf8_to_utf32_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(convert_utf8_to_utf16le_tests)
target_link_libraries(convert_utf8_to_utf16le_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(convert_utf8_to_utf16be_tests)
target_link_libraries(convert_utf8_to_utf16be_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(convert_utf8_to_utf16le_with_errors_tests)
target_link_libraries(convert_utf8_to_utf16le_with_errors_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(convert_utf8_to_utf16be_with_errors_tests)
target_link_libraries(convert_utf8_to_utf16be_with_errors_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(convert_utf8_to_utf32_tests)
target_link_libraries(convert_utf8_to_utf32_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(convert_utf8_to_utf32_with_errors_tests)
target_link_libraries(convert_utf8_to_utf32_with_errors_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(convert_utf16le_to_latin1_tests)
target_link_libraries(convert_utf16le_to_latin1_tests 
  PUBLIC simdutf::tests::helpers
          simdutf::tests::reference)

add_cpp_test(convert_utf16be_to_latin1_tests)
target_link_libraries(convert_utf16be_to_latin1_tests 
  PUBLIC simdutf::tests::helpers
          simdutf::tests::reference)

add_cpp_test(convert_utf16le_to_latin1_tests_with_errors)
target_link_libraries(convert_utf16le_to_latin1_tests_with_errors 
  PUBLIC simdutf::tests::helpers
          simdutf::tests::reference)

add_cpp_test(convert_utf16be_to_latin1_tests_with_errors)
target_link_libraries(convert_utf16be_to_latin1_tests_with_errors 
  PUBLIC simdutf::tests::helpers
          simdutf::tests::reference)

add_cpp_test(convert_valid_utf16le_to_latin1_tests)
target_link_libraries(convert_valid_utf16le_to_latin1_tests 
  PUBLIC simdutf::tests::helpers
          simdutf::tests::reference)

add_cpp_test(convert_valid_utf16be_to_latin1_tests)
target_link_libraries(convert_valid_utf16be_to_latin1_tests 
  PUBLIC simdutf::tests::helpers
          simdutf::tests::reference)

add_cpp_test(convert_utf16_to_utf8_safe_tests)
target_link_libraries(convert_utf16_to_utf8_safe_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(convert_utf16le_to_utf8_tests)
target_link_libraries(convert_utf16le_to_utf8_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(convert_utf16be_to_utf8_tests)
target_link_libraries(convert_utf16be_to_utf8_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(convert_utf16le_to_utf8_with_errors_tests)
target_link_libraries(convert_utf16le_to_utf8_with_errors_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(convert_utf16be_to_utf8_with_errors_tests)
target_link_libraries(convert_utf16be_to_utf8_with_errors_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)
        
add_cpp_test(convert_utf32_to_latin1_tests)
target_link_libraries(convert_utf32_to_latin1_tests 
  PUBLIC simdutf::tests::helpers
        simdutf::tests::reference)

add_cpp_test(convert_valid_utf32_to_latin1_tests)
target_link_libraries(convert_valid_utf32_to_latin1_tests 
  PUBLIC simdutf::tests::helpers
        simdutf::tests::reference)

add_cpp_test(convert_utf32_to_latin1_with_errors_tests)
target_link_libraries(convert_utf32_to_latin1_with_errors_tests 
  PUBLIC simdutf::tests::helpers
        simdutf::tests::reference)         
          
add_cpp_test(convert_utf32_to_utf8_tests)
target_link_libraries(convert_utf32_to_utf8_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(convert_utf32_to_utf8_with_errors_tests)
target_link_libraries(convert_utf32_to_utf8_with_errors_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(convert_utf32_to_utf16le_tests)
target_link_libraries(convert_utf32_to_utf16le_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(convert_utf32_to_utf16be_tests)
target_link_libraries(convert_utf32_to_utf16be_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(convert_utf32_to_utf16le_with_errors_tests)
target_link_libraries(convert_utf32_to_utf16le_with_errors_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(convert_utf32_to_utf16be_with_errors_tests)
target_link_libraries(convert_utf32_to_utf16be_with_errors_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(convert_valid_utf16le_to_utf8_tests)
target_link_libraries(convert_valid_utf16le_to_utf8_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(convert_valid_utf16be_to_utf8_tests)
target_link_libraries(convert_valid_utf16be_to_utf8_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(convert_valid_utf32_to_utf8_tests)
target_link_libraries(convert_valid_utf32_to_utf8_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(convert_valid_utf32_to_utf16le_tests)
target_link_libraries(convert_valid_utf32_to_utf16le_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(convert_valid_utf32_to_utf16be_tests)
target_link_libraries(convert_valid_utf32_to_utf16be_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(convert_utf16le_to_utf32_tests)
target_link_libraries(convert_utf16le_to_utf32_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(convert_utf16be_to_utf32_tests)
target_link_libraries(convert_utf16be_to_utf32_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(convert_utf16le_to_utf32_with_errors_tests)
target_link_libraries(convert_utf16le_to_utf32_with_errors_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(convert_utf16be_to_utf32_with_errors_tests)
target_link_libraries(convert_utf16be_to_utf32_with_errors_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(convert_valid_utf16le_to_utf32_tests)
target_link_libraries(convert_valid_utf16le_to_utf32_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(convert_valid_utf16be_to_utf32_tests)
target_link_libraries(convert_valid_utf16be_to_utf32_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(count_utf8)
target_link_libraries(count_utf8
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)
if(SIMDUTF_FAST_TESTS)
  target_compile_definitions(count_utf8 PRIVATE SIMDUTF_FAST_TESTS=1)
else()
  target_compile_definitions(count_utf8 PRIVATE SIMDUTF_FAST_TESTS=0)
endif()

add_cpp_test(count_utf16le)
target_link_libraries(count_utf16le
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(count_utf16be)
target_link_libraries(count_utf16be
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(detect_encodings_tests)
target_link_libraries(detect_encodings_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(text_encoding_tests)
target_link_libraries(text_encoding_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)
         
add_cpp_test(basic_fuzzer)
target_link_libraries(basic_fuzzer
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)
if(SIMDUTF_FAST_TESTS)
   target_compile_definitions(basic_fuzzer PRIVATE SIMDUTF_TEST_FUZZER_TRIALS=100)
else()
   target_compile_definitions(basic_fuzzer PRIVATE SIMDUTF_TEST_FUZZER_TRIALS=100000)
endif()

add_cpp_test(base64_tests)
target_link_libraries(base64_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)
if(SIMDUTF_FAST_TESTS)
   target_compile_definitions(base64_tests PRIVATE SIMDUTF_BASE64_TEST_MAXLEN=100)
else()
   target_compile_definitions(base64_tests PRIVATE SIMDUTF_BASE64_TEST_MAXLEN=2048)
endif()

add_cpp_test(constexpr_base64_tests)
target_link_libraries(constexpr_base64_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(span_tests)
target_link_libraries(span_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

# C API tests
add_cpp_test(simdutf_c_tests)
target_link_libraries(simdutf_c_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)
if(SIMDUTF_ATOMIC_BASE64_TESTS)
  add_cpp_test(atomic_base64_tests)
  target_link_libraries(atomic_base64_tests
    PUBLIC simdutf::tests::helpers
          simdutf::tests::reference)
endif(SIMDUTF_ATOMIC_BASE64_TESTS)
add_cpp_test(internal_tests)
target_link_libraries(internal_tests
  PUBLIC simdutf::tests::helpers)

add_cpp_test(utf8_length_from_utf16_tests)
target_link_libraries(utf8_length_from_utf16_tests
    PUBLIC simdutf::tests::helpers)

add_cpp_test(to_well_formed_utf16_tests)
target_link_libraries(to_well_formed_utf16_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)

add_cpp_test(fixed_string_tests)
target_link_libraries(fixed_string_tests
  PUBLIC simdutf::tests::helpers
         simdutf::tests::reference)


# test C++26 embed, currently only available in gcc 15
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
 if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "15")
   add_cpp_test(embed_tests)
   target_link_libraries(embed_tests
     PUBLIC simdutf::tests::helpers
            simdutf::tests::reference)
          target_compile_options(embed_tests PRIVATE --embed-directory ${CMAKE_CURRENT_SOURCE_DIR})
   endif()
endif()

find_program(GIT_EXECUTABLE git)
if(Python3_FOUND AND GIT_EXECUTABLE)
  add_test(NAME check_feature_macros
    COMMAND ${Python3_EXECUTABLE} ${SIMDUTF_SOURCE_DIR}/scripts/check_feature_macros.py
    WORKING_DIRECTORY ${SIMDUTF_SOURCE_DIR}
  )
endif()
