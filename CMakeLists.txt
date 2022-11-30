cmake_minimum_required(VERSION 3.15)

project(simdutf
  DESCRIPTION "Fast Unicode validation, transcoding and processing"
  LANGUAGES CXX
  VERSION 1.0.0
)

include(GNUInstallDirs)
# The following requires CMake 3.21.
# if(PROJECT_IS_TOP_LEVEL)
#  message(STATUS "Building simdutf as a top-level project.")
#  include(CTest)
#else()
#  message(STATUS "Building simdutf as a subproject.")
#endif(PROJECT_IS_TOP_LEVEL)
include(CTest)
include(cmake/simdutf-flags.cmake)

set(SIMDUTF_LIB_VERSION "2.0.4" CACHE STRING "simdutf library version")
set(SIMDUTF_LIB_SOVERSION "2" CACHE STRING "simdutf library soversion")
option(SIMDUTF_BENCHMARKS "Whether the benchmarks are included as part of the CMake Build." ON)
option(SIMDUTF_TOOLS "Whether the tools are included as part of the CMake build." ON)
option(SIMDUTF_ICONV "Whether to use iconv as part of the CMake build if available." ON)

set(SIMDUTF_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(src)
add_subdirectory(singleheader)

if (BUILD_TESTING)
  message(STATUS "The tests are enabled.")
  add_subdirectory(tests)
else()
  message(STATUS "The tests are disabled.")
endif(BUILD_TESTING)


if (SIMDUTF_TOOLS)
  if(CMAKE_CXX_COMPILER_ID MATCHES GNU AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 8.0)
    message(STATUS "The benchmark tool requires GCC 8.0 or better.")
  else()
    add_subdirectory(tools)
  endif()
endif()


if (SIMDUTF_BENCHMARKS)
  if((CMAKE_CXX_COMPILER_ID MATCHES GNU) AND (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "8.0"))
    message(STATUS "The benchmark tool requires GCC 8.0 or better.")
  else()
    message(STATUS "The benchmarks can be disabled by setting SIMDUTF_BENCHMARKS, e.g., -D SIMDUTF_BENCHMARKS=OFF.")
    add_subdirectory(benchmarks)
  endif()
else()
  message(STATUS "The benchmarks can be enabled by setting SIMDUTF_BENCHMARKS, e.g., -D SIMDUTF_BENCHMARKS=ON.")
endif()

# ---- Install rules ----
add_library(simdutf::simdutf ALIAS simdutf)

set_target_properties(
    simdutf PROPERTIES
    VERSION "${SIMDUTF_LIB_VERSION}"
    SOVERSION "${SIMDUTF_LIB_SOVERSION}"
    WINDOWS_EXPORT_ALL_SYMBOLS YES
)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

install(
    FILES include/simdutf.h
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    COMPONENT simdutf_Development
)

install(
    DIRECTORY include/simdutf
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    COMPONENT simdutf_Development
)

install(
    TARGETS simdutf
    EXPORT simdutfTargets
    RUNTIME COMPONENT simdutf_Runtime
    LIBRARY COMPONENT simdutf_Runtime
    NAMELINK_COMPONENT simdutf_Development
    ARCHIVE COMPONENT simdutf_Development
    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

configure_file(cmake/simdutf-config.cmake.in simdutf-config.cmake @ONLY)

write_basic_package_version_file(
    simdutf-config-version.cmake
    COMPATIBILITY SameMinorVersion
)

set(
    SIMDUTF_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/simdutf"
    CACHE STRING "CMake package config location relative to the install prefix"
)
mark_as_advanced(SIMDUTF_INSTALL_CMAKEDIR)

install(
    FILES
    "${PROJECT_BINARY_DIR}/simdutf-config.cmake"
    "${PROJECT_BINARY_DIR}/simdutf-config-version.cmake"
    DESTINATION "${SIMDUTF_INSTALL_CMAKEDIR}"
    COMPONENT simdutf_Development
)

install(
    EXPORT simdutfTargets
    NAMESPACE simdutf::
    DESTINATION "${SIMDUTF_INSTALL_CMAKEDIR}"
    COMPONENT example_Development
)

#
# CPack
#
if(is_top_project)
  set(CPACK_PACKAGE_VENDOR "Daniel Lemire")
  set(CPACK_PACKAGE_CONTACT "lemire@gmail.com")
  set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE-MIT")
  set(CPACK_RPM_PACKAGE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE-MIT")
  set(CPACK_RESOURCE_FILE_README "${PROJECT_SOURCE_DIR}/README.md")
  set(CPACK_SOURCE_GENERATOR "TGZ;ZIP")
  include(CPack)
endif()

# ----
