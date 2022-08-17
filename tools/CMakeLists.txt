cmake_minimum_required(VERSION 3.15)

add_executable(sutf sutf.cpp)

find_package(Iconv)
if(Iconv_FOUND)
  message(STATUS "Iconv was found!")
  target_compile_definitions(sutf PRIVATE ICONV_AVAILABLE=1)
  if(Iconv_IS_BUILT_IN)
    message(STATUS "Iconv is part of the C library.")
  else(Iconv_IS_BUILT_IN)
    message(STATUS "Iconv is a separate library, headers at ${Iconv_INCLUDE_DIR}")
    target_link_libraries(sutf PUBLIC  ${Iconv_LIBRARY})
    target_include_directories(sutf PUBLIC ${Iconv_INCLUDE_DIR})
  endif(Iconv_IS_BUILT_IN)
else(Iconv_FOUND)
  message(STATUS "Iconv was not found!")
endif(Iconv_FOUND)

if((CMAKE_CXX_COMPILER_ID STREQUAL "GNU") OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
   target_link_options(sutf PRIVATE "-static-libstdc++")
   target_link_options(sutf PRIVATE "-Wl,--gc-sections")
 endif()

set_property(TARGET sutf PROPERTY CXX_STANDARD 17)
set_property(TARGET sutf PROPERTY CXX_STANDARD_REQUIRED ON)

target_link_libraries(sutf PUBLIC simdutf)