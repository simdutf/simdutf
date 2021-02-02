#
# Provides the simdutf headers.
#
# target_link_libraries(my-project simdutf-headers) grants the headers. It does not provide the
# source, libraries or any compiler flags.
#
add_library(simdutf-headers INTERFACE)
target_compile_features(simdutf-headers INTERFACE cxx_std_11) # headers require at least C++11
target_include_directories(simdutf-headers INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCDIR}>)

install(TARGETS simdutf-headers EXPORT simdutf-config INCLUDES DESTINATION include)
