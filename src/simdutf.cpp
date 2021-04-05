#include "simdutf.h"
#include "implementation.cpp"
#include "encoding_types.cpp"

SIMDUTF_PUSH_DISABLE_WARNINGS
SIMDUTF_DISABLE_UNDESIRED_WARNINGS


#if SIMDUTF_IMPLEMENTATION_ARM64
#include "arm64/implementation.cpp"
#endif
#if SIMDUTF_IMPLEMENTATION_FALLBACK
#include "fallback/implementation.cpp"
#endif
#if SIMDUTF_IMPLEMENTATION_HASWELL
#include "haswell/implementation.cpp"
#endif
#if SIMDUTF_IMPLEMENTATION_PPC64
#include "ppc64/implementation.cpp"
#endif
#if SIMDUTF_IMPLEMENTATION_WESTMERE
#include "westmere/implementation.cpp"
#endif

SIMDUTF_POP_DISABLE_WARNINGS
