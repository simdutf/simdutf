#ifndef SIMDUTF_AVX512BW_H
#define SIMDUTF_AVX512BW_H

#ifdef SIMDUTF_HASWELL_H
#error "avx512bw.h must be included before haswell.h"
#endif
#ifdef SIMDUTF_WESTMERE_H
#error "avx512bw.h must be included before westmere.h"
#endif
#ifdef SIMDUTF_FALLBACK_H
#error "avx512bw.h must be included before fallback.h"
#endif

#include "simdutf/portability.h"
// An AVX512 implementation can be determined only at runtime.
#ifndef SIMDUTF_IMPLEMENTATION_AVX512BW
#define SIMDUTF_IMPLEMENTATION_AVX512BW (SIMDUTF_IS_X86_64)
#endif


// To see why  (__BMI__) && (__PCLMUL__) && (__LZCNT__) are not part of this next line, see
// https://github.com/simdutf/simdutf/issues/1247
#define SIMDUTF_CAN_ALWAYS_RUN_AVX512BW ((SIMDUTF_IMPLEMENTATION_AVX512BW) && (SIMDUTF_IS_X86_64) && (__AVX2__))

#if SIMDUTF_IMPLEMENTATION_AVX512BW

#if SIMDUTF_CAN_ALWAYS_RUN_AVX512BW
#define SIMDUTF_TARGET_AVX512BW
#define SIMDJSON_UNTARGET_AVX512BW
#else
#define SIMDUTF_TARGET_AVX512BW SIMDUTF_TARGET_REGION("avx512f,avx512dq,avx512cd,avx512bw,avx512vl,avx2,bmi,pclmul,lzcnt")
#define SIMDJSON_UNTARGET_AVX512BW SIMDJSON_UNTARGET_REGION
#endif

namespace simdutf {
namespace avx512bw {
} // namespace avx512bw
} // namespace simdutf

#include "simdutf/avx512bw/intrinsics.h"
#include "simdutf/avx512bw/implementation.h"

//#include "simdutf/avx512bw/begin.h"
//#include "simdutf/avx512bw/end.h"

#endif // SIMDUTF_IMPLEMENTATION_AVX512BW
#endif // SIMDUTF_AVX512BW_H
