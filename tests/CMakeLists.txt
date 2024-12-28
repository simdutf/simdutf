set(SIMDUTF_TEST_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../)
if(MSVC)
  add_compile_options("/Zi" "/EHsc" "/GR")
endif()

include(${PROJECT_SOURCE_DIR}/cmake/add_cpp_test.cmake)

add_cpp_test(validate_utf16le_with_errors_tests)
target_link_libraries(validate_utf16le_with_errors_tests PRIVATE simdutf)
