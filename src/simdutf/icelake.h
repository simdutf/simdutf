#ifndef SIMDUTF_ICELAKE_H
#define SIMDUTF_ICELAKE_H


#include "simdutf/portability.h"
// We allow icelake on x64.
#ifndef SIMDUTF_IMPLEMENTATION_ICELAKE
#define SIMDUTF_IMPLEMENTATION_ICELAKE (SIMDUTF_IS_X86_64)
#endif


// To see why  (__BMI__) && (__PCLMUL__) && (__LZCNT__) are not part of this next line, see
// https://github.com/simdutf/simdutf/issues/1247
#define SIMDUTF_CAN_ALWAYS_RUN_ICELAKE ((SIMDUTF_IMPLEMENTATION_ICELAKE) && (SIMDUTF_IS_X86_64) && (__AVX2__) && (SIMDUTF_HAS_AVX512F && \
                                         SIMDUTF_HAS_AVX512DQ && \
                                         SIMDUTF_HAS_AVX512VL && \
                                           SIMDUTF_HAS_AVX512VBMI2))

#if SIMDUTF_IMPLEMENTATION_ICELAKE
#if SIMDUTF_CAN_ALWAYS_RUN_ICELAKE
#define SIMDUTF_TARGET_ICELAKE
#define SIMDJSON_UNTARGET_ICELAKE
#else
#define SIMDUTF_TARGET_ICELAKE SIMDUTF_TARGET_REGION("avx512f,avx512dq,avx512cd,avx512bw,avx512vbmi,avx512vbmi2,avx512vl,avx2,bmi,pclmul,lzcnt")
#define SIMDUTF_UNTARGET_ICELAKE SIMDUTF_UNTARGET_REGION
#endif

namespace simdutf {
namespace icelake {
} // namespace icelake
} // namespace simdutf



//
// These two need to be included outside SIMDUTF_TARGET_REGION
//
#include "simdutf/icelake/intrinsics.h"
#include "simdutf/icelake/implementation.h"

//
// The rest need to be inside the region
//
//#include "simdutf/icelake/begin.h"
//#include "simdutf/icelake/end.h"



#endif // SIMDUTF_IMPLEMENTATION_ICELAKE
#endif // SIMDUTF_ICELAKE_H
