#ifndef SIMDUTF_COMPILER_CHECK_H
#define SIMDUTF_COMPILER_CHECK_H

#ifndef __cplusplus
  #error simdutf requires a C++ compiler
#endif

#ifndef SIMDUTF_CPLUSPLUS
  #if defined(_MSVC_LANG) && !defined(__clang__)
    #define SIMDUTF_CPLUSPLUS (_MSC_VER == 1900 ? 201103L : _MSVC_LANG)
  #else
    #define SIMDUTF_CPLUSPLUS __cplusplus
  #endif
#endif

// C++ 26
#if !defined(SIMDUTF_CPLUSPLUS26) && (SIMDUTF_CPLUSPLUS >= 202602L)
  #define SIMDUTF_CPLUSPLUS26 1
#endif

// C++ 23
// AppleClang reports C++23 features but doesn't update __cplusplus correctly,
// so we check for the if consteval feature test macro as a fallback.
#if !defined(SIMDUTF_CPLUSPLUS23) &&                                           \
    ((SIMDUTF_CPLUSPLUS >= 202302L) ||                                         \
     (defined(__cpp_if_consteval) && __cpp_if_consteval >= 202106L))
  #define SIMDUTF_CPLUSPLUS23 1
#endif

// C++ 20
#if !defined(SIMDUTF_CPLUSPLUS20) && (SIMDUTF_CPLUSPLUS >= 202002L)
  #define SIMDUTF_CPLUSPLUS20 1
#endif

// C++ 17
#if !defined(SIMDUTF_CPLUSPLUS17) && (SIMDUTF_CPLUSPLUS >= 201703L)
  #define SIMDUTF_CPLUSPLUS17 1
#endif

// C++ 14
#if !defined(SIMDUTF_CPLUSPLUS14) && (SIMDUTF_CPLUSPLUS >= 201402L)
  #define SIMDUTF_CPLUSPLUS14 1
#endif

// C++ 11
#if !defined(SIMDUTF_CPLUSPLUS11) && (SIMDUTF_CPLUSPLUS >= 201103L)
  #define SIMDUTF_CPLUSPLUS11 1
#endif

#ifndef SIMDUTF_CPLUSPLUS11
  #error simdutf requires a compiler compliant with the C++11 standard
#endif

#endif // SIMDUTF_COMPILER_CHECK_H
