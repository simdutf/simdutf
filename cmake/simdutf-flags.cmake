

if (NOT CMAKE_BUILD_TYPE)
  message(STATUS "No build type selected, default to Release")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  if(SIMDUTF_SANITIZE)
    message(WARNING "No build type selected and you have enabled the sanitizer. Consider setting CMAKE_BUILD_TYPE to Debug to help identify the eventual problems.")
  endif()
endif()

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/tools/cmake")

# We compile tools, tests, etc. with C++ 17. Override yourself if you need on a target.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MACOSX_RPATH OFF)
