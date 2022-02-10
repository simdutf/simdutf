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

#if defined(SIMDUTF_CPLUSPLUS17)
#if !__has_include(<atomic>)
#define SIMDUTF_NO_THREADS 1
#endif
#endif

#endif // SIMDUTF_COMPILER_CHECK_H
