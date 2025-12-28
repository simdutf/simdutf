#ifndef SIMDUTF_LSX_H
#define SIMDUTF_LSX_H

#ifdef SIMDUTF_FALLBACK_H
  #error "lsx.h must be included before fallback.h"
#endif

#ifndef SIMDUTF_CAN_ALWAYS_RUN_LASX
  #error "lsx.h must be included after lasx.h"
#endif

#include "simdutf/portability.h"

#ifndef SIMDUTF_IMPLEMENTATION_LSX
  #if SIMDUTF_CAN_ALWAYS_RUN_LASX
    #define SIMDUTF_IMPLEMENTATION_LSX 0
  #else
    #define SIMDUTF_IMPLEMENTATION_LSX (SIMDUTF_IS_LSX)
  #endif
#endif
#if SIMDUTF_IMPLEMENTATION_LSX && SIMDUTF_IS_LSX
  #define SIMDUTF_CAN_ALWAYS_RUN_LSX 1
#else
  #define SIMDUTF_CAN_ALWAYS_RUN_LSX 0
#endif

#define SIMDUTF_CAN_ALWAYS_RUN_FALLBACK (SIMDUTF_IMPLEMENTATION_FALLBACK)
#include "simdutf/internal/isadetection.h"

#if SIMDUTF_IMPLEMENTATION_LSX
  #define SIMDUTF_TARGET_LSX SIMDUTF_TARGET_REGION("lsx")

namespace simdutf {
/**
 * Implementation for LoongArch SX.
 */
namespace lsx {} // namespace lsx
} // namespace simdutf

  #include "simdutf/lsx/implementation.h"

  #include "simdutf/lsx/begin.h"

  // Declarations
  #include "simdutf/lsx/intrinsics.h"
  #include "simdutf/lsx/bitmanipulation.h"
  #include "simdutf/lsx/simd.h"

  #include "simdutf/lsx/end.h"

#endif // SIMDUTF_IMPLEMENTATION_LSX

#endif // SIMDUTF_LSX_H
