#
# For callers who intend to #include simdutf.cpp.
#
# target_link_libraries(my-program simdutf-include-source) gives you the header and source
# directories. It does not specify any compiler flags.
#

add_library(simdutf-include-source INTERFACE)
target_link_libraries(simdutf-include-source INTERFACE simdutf-headers)
target_include_directories(simdutf-include-source INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
# If a CMake user installs simdutf and then does...
#   find_package(simdutf CONFIG REQUIRED)
#   find_package(Threads REQUIRED)
#   target_link_libraries(test PRIVATE simdutf-include-source)
# It is not clear what it should do? Does the user get access to the src files?
# install(TARGETS simdutf-include-source EXPORT simdutf-config)

#
# For callers who intend to compile simdutf.cpp themselves.
#
# target_link_libraries(my-object simdutf-source) gives you the header and source directories, plus
# the .cpp sources. It does not specify any compiler flags.
#
add_library(simdutf-source INTERFACE)
target_sources(simdutf-source INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/simdutf.cpp)
target_link_libraries(simdutf-source INTERFACE simdutf-include-source)
# Note that simdutf.cpp is *not* installed so installing simdutf-include-source is not great.
# If a CMake user installs simdutf and then does...
#   find_package(simdutf CONFIG REQUIRED)
#   find_package(Threads REQUIRED)
#   target_link_libraries(test PRIVATE simdutf-source)
# It might fail with  Cannot find source file: simdutf.cpp
# See issue https://github.com/simdutf/simdutf/issues/1383
# install(TARGETS simdutf-source EXPORT simdutf-config)

#
# simdutf is the distributed library compiled with flags.
#
# target_link_libraries(my-object simdutf) gives you the .so or .a to link against, plus the header
# directory. It does not specify any compiler flags, even though simdutf.so/a was compiled with
# target_link_libraries(simdutf PRIVATE simdutf-flags).
#

if(SIMDUTF_BUILD_STATIC)
  MESSAGE( STATUS "Building a static library." )
  ###
  # In the case of a static library, if you do "target_link_libraries(simdutf PRIVATE simdutf-source simdutf-internal-flags)"
  # without also calling "install(TARGETS simdutf-source EXPORT simdutf-config)" you get an error
  # to the effect that you are trying to install 'simdutf' which depends on 'simdutf-source' while
  # not installing 'simdutf-source', so it fails. So we bypass entirely simdutf-source since we do
  # not want to install them.
  ####
  add_library(simdutf STATIC simdutf.cpp)
  target_include_directories(simdutf PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
else()
  MESSAGE( STATUS "Building a dynamic library." )
  add_library(simdutf SHARED "")
  target_compile_definitions(simdutf INTERFACE SIMDUTF_USING_LIBRARY=1)
  if(MSVC)
    MESSAGE( STATUS "Building a Windows DLL using Visual Studio, exporting all symbols automatically." )
    set_target_properties(simdutf PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS 1)
  endif()
  ###
  # Somehow, for a dynamic library, the next line is entirely fine (but not for a static one).
  ###
  target_link_libraries(simdutf PRIVATE simdutf-source simdutf-internal-flags)
endif()
target_link_libraries(simdutf PUBLIC simdutf-headers simdutf-flags) # Only expose the headers, not sources

##
## In systems like R, libraries must not use stderr or abort to be acceptable.
## Thus we make it a hard rule that one is not allowed to call abort or stderr.
## The sanitized builds are allowed to abort.
##
if(NOT SIMDUTF_SANITIZE)
  find_program(GREP grep)
  find_program(NM nm)
  if((NOT GREP) OR (NOT NM))
    message("grep and nm are unavailable on this system.")
  else()
    add_test(
      NAME "avoid_abort"
      # Under FreeBSD, the __cxa_guard_abort symbol may appear but it is fine.
      # So we want to look for <space><possibly _>abort as a test.
      COMMAND sh -c "${NM}  $<TARGET_FILE_NAME:simdutf> |  ${GREP} ' _*abort' || exit 0  && exit 1"
      WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    )
    add_test(
      NAME "avoid_cout"
      COMMAND sh -c "${NM}  $<TARGET_FILE_NAME:simdutf> |  ${GREP} ' _*cout' || exit 0  && exit 1"
      WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    )
    add_test(
      NAME "avoid_cerr"
      COMMAND sh -c "${NM}  $<TARGET_FILE_NAME:simdutf> |  ${GREP} ' _*cerr' || exit 0  && exit 1"
      WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    )
    add_test(
      NAME "avoid_printf"
      COMMAND sh -c "${NM}  $<TARGET_FILE_NAME:simdutf> |  ${GREP} ' _*printf' || exit 0  && exit 1"
      WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    )
    add_test(
      NAME "avoid_stdout"
      COMMAND sh -c "${NM}  $<TARGET_FILE_NAME:simdutf> |  ${GREP} stdout || exit 0 && exit 1"
      WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    )
    add_test(
      NAME "avoid_stderr"
      COMMAND sh -c "${NM}  $<TARGET_FILE_NAME:simdutf> |  ${GREP} stderr || exit 0 && exit 1"
      WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    )
  endif()
endif()

if(NOT MSVC)
  ## We output the library at the root of the current directory where cmake is invoked
  ## This is handy but Visual Studio will happily ignore us
  set_target_properties(simdutf PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
  MESSAGE( STATUS "Library output directory: " ${PROJECT_BINARY_DIR})

  ############
  # Please do not delete the following, our users want version numbers. See
  # https://github.com/simdutf/simdutf/issues/1014
  # https://github.com/simdutf/simdutf/issues/52
  ###########
  set_target_properties(simdutf PROPERTIES VERSION ${SIMDUTF_LIB_VERSION}	SOVERSION ${SIMDUTF_LIB_SOVERSION})
  ##########
  # End of the do-not-delete message.
  #########
endif()

#
# Installation
#
install(TARGETS simdutf
  EXPORT simdutf-config
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(EXPORT simdutf-config
  FILE simdutf-targets.cmake
  NAMESPACE simdutf::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/simdutf
)
