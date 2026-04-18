add_library(simdutf-include-source INTERFACE)
target_include_directories(simdutf-include-source INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
add_library(simdutf-source INTERFACE)
target_sources(simdutf-source INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/simdutf.cpp)
target_link_libraries(simdutf-source INTERFACE simdutf-include-source)
add_library(simdutf simdutf.cpp)
if(WIN32 AND BUILD_SHARED_LIBS)
  target_compile_definitions(
      simdutf
      PRIVATE SIMDUTF_BUILDING_WINDOWS_DYNAMIC_LIBRARY=1
      INTERFACE SIMDUTF_USING_WINDOWS_DYNAMIC_LIBRARY=1
  )
endif()
if(SIMDUTF_LOGGING)
  target_compile_definitions(simdutf PUBLIC SIMDUTF_LOGGING=1)
endif()
if(FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION)
  target_compile_definitions(simdutf PUBLIC FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION=1)
endif()
target_include_directories(simdutf PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> )
target_include_directories(simdutf PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>")
if(NOT DEFINED CMAKE_POSITION_INDEPENDENT_CODE)
  # We default to ON for all targets, so that we can use the library in shared libraries.
  set_target_properties(simdutf PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif(NOT DEFINED CMAKE_POSITION_INDEPENDENT_CODE)
if(MSVC)
  if("${MSVC_TOOLSET_VERSION}" STREQUAL "140")
    target_compile_options(simdutf PRIVATE /W0 /sdl)
    set(SIMDUTF_LEGACY_VISUAL_STUDIO TRUE)
  else()
    target_compile_options(simdutf PRIVATE /WX /W3 /sdl /w34714) # https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-4-c4714?view=vs-2019
  endif()
else(MSVC)
  target_compile_options(simdutf PRIVATE -Wall -Wextra -Weffc++)
  target_compile_options(simdutf PRIVATE -Wsign-compare -Wshadow -Wwrite-strings -Wpointer-arith -Winit-self -Wconversion -Wno-sign-conversion -Wunused-function)
  target_compile_options(simdutf PRIVATE -Wsuggest-override)
  target_compile_options(simdutf PRIVATE -Wfatal-errors)
endif(MSVC)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_compile_options(simdutf PRIVATE -fdiagnostics-color=always)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    target_compile_options(simdutf PRIVATE -fcolor-diagnostics)
endif()

# workaround for GNU GCC poor AVX load/store code generation
if ((CMAKE_CXX_COMPILER_ID STREQUAL "GNU") AND (CMAKE_SYSTEM_PROCESSOR MATCHES "^(i.86|x86(_64)?)$"))
  target_compile_options(simdutf PRIVATE -mno-avx256-split-unaligned-load -mno-avx256-split-unaligned-store)
endif()

if ((CMAKE_CXX_COMPILER_ID STREQUAL "GNU") AND (CMAKE_SYSTEM STREQUAL "PowerPC"))
  target_compile_options(simdutf PRIVATE -maltivec -mvsx -mcpu=power8)
endif()

##
## Feature-specific library variants for the tools.
## These are NOT installed — they exist only to give each tool a smaller binary.
##
if(SIMDUTF_TOOLS)
  # --- simdutf-base64: only base64 support (for fastbase64) ---
  add_library(simdutf-base64 STATIC simdutf.cpp)
  target_include_directories(simdutf-base64 PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
  target_include_directories(simdutf-base64 PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>")
  target_compile_definitions(simdutf-base64 PRIVATE
    SIMDUTF_FEATURE_BASE64=1
    SIMDUTF_FEATURE_UTF8=0
    SIMDUTF_FEATURE_UTF16=0
    SIMDUTF_FEATURE_UTF32=0
    SIMDUTF_FEATURE_ASCII=0
    SIMDUTF_FEATURE_LATIN1=0
    SIMDUTF_FEATURE_DETECT_ENCODING=0
  )
  set_target_properties(simdutf-base64 PROPERTIES POSITION_INDEPENDENT_CODE ON)

  # --- simdutf-nobase64: everything except base64 (for sutf) ---
  add_library(simdutf-nobase64 STATIC simdutf.cpp)
  target_include_directories(simdutf-nobase64 PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
  target_include_directories(simdutf-nobase64 PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>")
  target_compile_definitions(simdutf-nobase64 PRIVATE
    SIMDUTF_FEATURE_BASE64=0
    SIMDUTF_FEATURE_UTF8=1
    SIMDUTF_FEATURE_UTF16=1
    SIMDUTF_FEATURE_UTF32=1
    SIMDUTF_FEATURE_ASCII=1
    SIMDUTF_FEATURE_LATIN1=1
    SIMDUTF_FEATURE_DETECT_ENCODING=0
  )
  set_target_properties(simdutf-nobase64 PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

if(SIMDUTF_ALWAYS_INCLUDE_FALLBACK)
  message(STATUS "SIMDUTF_ALWAYS_INCLUDE_FALLBACK is set to ${SIMDUTF_ALWAYS_INCLUDE_FALLBACK}")
  target_compile_definitions(simdutf PRIVATE SIMDUTF_IMPLEMENTATION_FALLBACK=1)
endif()

include(CheckCXXCompilerFlag)
include(CheckCXXSourceCompiles)

check_cxx_compiler_flag("-fno-exceptions" HAVE_CXX_FLAG_NO_EXCEPTIONS)
check_cxx_compiler_flag("-fno-rtti" HAVE_CXX_FLAG_NO_RTTI)

# CheckCXXLinkerFlag is only available in CMake 3.18+, so we probe the linker
# flag directly via CheckCXXSourceCompiles + CMAKE_REQUIRED_LINK_OPTIONS.
set(_simdutf_prev_link_options "${CMAKE_REQUIRED_LINK_OPTIONS}")
set(CMAKE_REQUIRED_LINK_OPTIONS "-nostdlib++")
check_cxx_source_compiles("int main() { return 0; }"
  HAVE_CXX_LINKER_FLAG_NO_STDLIBXX)
set(CMAKE_REQUIRED_LINK_OPTIONS "${_simdutf_prev_link_options}")
unset(_simdutf_prev_link_options)

##
## simdutf-nostdlibcxx: a variant of the library that does not require the C++
## standard library runtime (libc++ / libc++abi / libstdc++). It is intended to
## be consumed through the C interface (simdutf_c.h) and only supports GCC and
## Clang.
if(HAVE_CXX_FLAG_NO_EXCEPTIONS AND HAVE_CXX_FLAG_NO_RTTI AND HAVE_CXX_LINKER_FLAG_NO_STDLIBXX)
  add_library(simdutf-nostdlibcxx STATIC simdutf.cpp)
  target_include_directories(simdutf-nostdlibcxx PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
  target_include_directories(simdutf-nostdlibcxx PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>")
  target_compile_definitions(simdutf-nostdlibcxx PRIVATE SIMDUTF_NO_LIBCXX=1)
  target_compile_options(simdutf-nostdlibcxx PRIVATE
    -fno-exceptions -fno-rtti)
  set_target_properties(simdutf-nostdlibcxx PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

if(SIMDUTF_SANITIZE)
  target_compile_options(simdutf PUBLIC -fsanitize=address  -fno-omit-frame-pointer -fno-sanitize-recover=all)
  target_compile_definitions(simdutf PUBLIC ASAN_OPTIONS=detect_leaks=1)
  target_link_libraries(simdutf PUBLIC -fsanitize=address  -fno-omit-frame-pointer -fno-sanitize-recover=all)
endif()
if(SIMDUTF_SANITIZE_UNDEFINED)
  target_compile_options(simdutf PUBLIC -fsanitize=undefined -fno-sanitize-recover=all)
  target_link_libraries(simdutf PUBLIC -fsanitize=undefined)
endif()
if(SIMDUTF_SANITIZE_THREAD)
    target_compile_options(simdutf PUBLIC -fsanitize=thread)
    target_link_libraries(simdutf PUBLIC -fsanitize=thread)
endif()
if(SIMDUTF_INTERNAL_TESTS)
    target_compile_definitions(simdutf PUBLIC SIMDUTF_INTERNAL_TESTS)
endif()
if(WIN32 AND BUILD_SHARED_LIBS)
  set(SIMDUTF_WINDOWS_DLL TRUE)
endif()
