#ifndef SIMDUTF_HASWELL_H
#define SIMDUTF_HASWELL_H

#ifdef SIMDUTF_WESTMERE_H
#error "haswell.h must be included before westmere.h"
#endif
#ifdef SIMDUTF_FALLBACK_H
#error "haswell.h must be included before fallback.h"
#endif

#include "simdutf/portability.h"

// Default Haswell to on if this is x86-64. Even if we're not compiled for it, it could be selected
// at runtime.
#ifndef SIMDUTF_IMPLEMENTATION_HASWELL
#define SIMDUTF_IMPLEMENTATION_HASWELL (SIMDUTF_IS_X86_64)
#endif
// To see why  (__BMI__) && (__PCLMUL__) && (__LZCNT__) are not part of this next line, see
// https://github.com/simdutf/simdutf/issues/1247
#define SIMDUTF_CAN_ALWAYS_RUN_HASWELL ((SIMDUTF_IMPLEMENTATION_HASWELL) && (SIMDUTF_IS_X86_64) && (__AVX2__))

#if SIMDUTF_IMPLEMENTATION_HASWELL

#define SIMDUTF_TARGET_HASWELL SIMDUTF_TARGET_REGION("avx2,bmi,pclmul,lzcnt")

namespace simdutf {
/**
 * Implementation for Haswell (Intel AVX2).
 */
namespace haswell {
} // namespace haswell
} // namespace simdutf

//
// These two need to be included outside SIMDUTF_TARGET_REGION
//
#include "simdutf/haswell/implementation.h"
#include "simdutf/haswell/intrinsics.h"

//
// The rest need to be inside the region
//
#include "simdutf/haswell/begin.h"

// Declarations
#include "simdutf/haswell/bitmanipulation.h"
#include "simdutf/haswell/bitmask.h"
#include "simdutf/haswell/simd.h"

#include "simdutf/haswell/end.h"

#endif // SIMDUTF_IMPLEMENTATION_HASWELL
#endif // SIMDUTF_HASWELL_COMMON_H
