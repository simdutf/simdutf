#
# Amalgamation
#

set(SINGLEHEADER_FILES
  ${CMAKE_CURRENT_BINARY_DIR}/simdutf.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/simdutf.h
  ${CMAKE_CURRENT_BINARY_DIR}/amalgamation_demo.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/README.md
)
set(SINGLEHEADER_REPO_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/simdutf.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/simdutf.h
  ${CMAKE_CURRENT_SOURCE_DIR}/amalgamation_demo.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/README.md
)
set_source_files_properties(${SINGLEHEADER_FILES} PROPERTIES GENERATED TRUE)

find_program(BASH bash)

# Under Windows, exectuting a bash script works, except that you cannot generally
# do bash C:/path to my script. You need a  "mounted" path: /mnt/c/path
find_package(Python3 COMPONENTS Interpreter)

if (Python3_Interpreter_FOUND AND (NOT WIN32))
  add_custom_command(
    OUTPUT ${SINGLEHEADER_FILES}
    COMMAND ${CMAKE_COMMAND} -E env
      AMALGAMATE_SOURCE_PATH=${PROJECT_SOURCE_DIR}/src
      AMALGAMATE_INPUT_PATH=${PROJECT_SOURCE_DIR}/include
      AMALGAMATE_OUTPUT_PATH=${CMAKE_CURRENT_BINARY_DIR}
      ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/amalgamate.py
      #
      # This is the best way I could find to make amalgamation trigger whenever source files or
      # header files change: since the "simdutf" library has to get rebuilt when that happens, we
      # take a dependency on the generated library file (even though we're not using it). Depending
      # on simdutf-source doesn't do the trick because DEPENDS here can only depend on an
      # *artifact*--it won't scan source and include files the way a concrete library or executable
      # will.
      #
      # It sucks that we have to build the actual library to make it happen, but it's better than\
      # nothing!
      #
      DEPENDS amalgamate.py simdutf
  )

  ##
  # This is used by "make amalgamate" to update the original source files.
  # You can invoke it as cmake --build . --target amalgamate
  # We obviously don't do
  # this if source and generated files are in the same place--cmake gets mad!
  if (NOT (${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_BINARY_DIR}))
    add_custom_target(amalgamate)
    add_custom_command(TARGET amalgamate
      # We don't want CMake to know that it is writing to the source directory. No magic
      # file regeneration in the source directory without the user's knowledge.
      # OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/simdutf.cpp ${CMAKE_CURRENT_SOURCE_DIR}/simdutf.h ${CMAKE_CURRENT_SOURCE_DIR}/amalgamation_demo.cpp ${CMAKE_CURRENT_SOURCE_DIR}/README.md
      COMMAND ${CMAKE_COMMAND} -E copy ${SINGLEHEADER_FILES} ${CMAKE_CURRENT_SOURCE_DIR}
      DEPENDS ${SINGLEHEADER_FILES}
    )
  endif()


  ##
  # Adding the ability for CMake to modify the source is problematic. In particular, it will
  # happily regenerate the source files that are missing, silently. We do not want to do this.
  # If they are missing source files, the build should fail. You should not get silent patching
  # by CMake. The user can easily regenerate the files, deliberately.
  #
  # DO NOT DO THIS:
  # add_custom_target(amalgamate DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/simdutf.cpp ${CMAKE_CURRENT_SOURCE_DIR}/simdutf.h ${CMAKE_CURRENT_SOURCE_DIR}/amalgamation_demo.cpp ${CMAKE_CURRENT_SOURCE_DIR}/README.md)
  ##

else()

  # We do not have python3, so we use existing amalgamated files instead of generating them ...
  # (Do not do this if the source and destination are the same!)
  if (NOT (${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_BINARY_DIR}))
    add_custom_command(
      OUTPUT ${SINGLEHEADER_FILES}
      COMMAND ${CMAKE_COMMAND} -E copy
        ${SINGLEHEADER_REPO_FILES}
        ${CMAKE_CURRENT_BINARY_DIR}
      DEPENDS ${SINGLEHEADER_REPO_FILES}
    )
  endif()

endif()

add_custom_target(singleheader_tests)

#
# Do not depend on singleheader files directly: depend on this target instead.
# Otherwise the custom command may get triggered multiple times and race with itself!
#
add_custom_target(singleheader-files DEPENDS ${SINGLEHEADER_FILES})

#
# Include this if you intend to #include "simdutf.cpp" in your own .cpp files.
#
add_library(simdutf-singleheader-include-source INTERFACE)
target_include_directories(simdutf-singleheader-include-source INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)
add_dependencies(simdutf-singleheader-include-source singleheader-files)

#
# Include this to get "simdutf.cpp" included in your project as one of the sources.
#
add_library(simdutf-singleheader-source INTERFACE)
target_sources(simdutf-singleheader-source INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/simdutf.cpp>)
target_link_libraries(simdutf-singleheader-source INTERFACE simdutf-singleheader-include-source)

#
# We do not want CMake to update the original source files automatically "in passing" as
# part of a test that generates the files anew. This kind of side-effect is  bad.
#
# add_dependencies(simdutf-singleheader-source amalgamate) <=== NO NO NO
#
#

function(add_compile_only_test TEST_NAME)
  add_test(
    NAME ${TEST_NAME}
    COMMAND ${CMAKE_COMMAND} --build . --target ${TEST_NAME} --config $<CONFIGURATION>
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
  )
  set_target_properties(${TEST_NAME} PROPERTIES EXCLUDE_FROM_ALL TRUE EXCLUDE_FROM_DEFAULT_BUILD TRUE)
endfunction()

#
# Test the generated simdutf.cpp/simdutf.h using the generated amalgamation_demo.cpp
#
# Under Windows you should not mix static and dynamic. Pick one. The following test is static.
if(NOT SIMDUTF_LEGACY_VISUAL_STUDIO AND NOT SIMDUTF_WINDOWS_DLL)
  add_executable(amalgamation_demo $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/amalgamation_demo.cpp>)
  target_link_libraries(amalgamation_demo simdutf-singleheader-include-source)
  add_test(amalgamation_demo amalgamation_demo ${EXAMPLE_JSON} ${EXAMPLE_NDJSON})
  set_property(TEST amalgamation_demo APPEND PROPERTY LABELS per_implementation singleheader)
  add_dependencies(singleheader_tests amalgamation_demo)
  MESSAGE( STATUS "Including amalgamation_demo test. ${SIMDUTF_WINDOWS_DLL}" )
else()
  MESSAGE( STATUS "You either have an old Visual Studio or you are building a DLL, amalgamation_demo test disabled." )
endif()

# Under Windows you should not mix static and dynamic. Pick one. The following test is static.
if(NOT SIMDUTF_LEGACY_VISUAL_STUDIO AND NOT SIMDUTF_WINDOWS_DLL)
  add_library(simdutf-singleheader STATIC "")
  target_link_libraries(simdutf-singleheader simdutf-singleheader-source)
  add_compile_only_test(simdutf-singleheader)
  set_property(TEST simdutf-singleheader APPEND PROPERTY LABELS per_implementation singleheader)
  add_dependencies(singleheader_tests simdutf-singleheader)
  MESSAGE( STATUS "Including simdutf-singleheader test." )
else()
  MESSAGE( STATUS "You either have an old Visual Studio or you are building a DLL, simdutf-singleheader test disabled." )
endif()