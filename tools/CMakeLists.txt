cmake_minimum_required(VERSION 3.15)

add_executable(sutf sutf.cpp)
target_link_libraries(sutf PUBLIC simdutf)

if((CMAKE_CXX_COMPILER_ID STREQUAL "GNU") OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
  target_link_options(sutf PRIVATE "-static-libstdc++")
endif()

set_property(TARGET sutf PROPERTY CXX_STANDARD 17)
set_property(TARGET sutf PROPERTY CXX_STANDARD_REQUIRED ON)
