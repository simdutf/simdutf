#ifndef SIMDUTF_AVX512BW_H
#define SIMDUTF_AVX512BW_H

#include "simdutf/portability.h"

// An AVX512 implementation can be determined only in runtime.
#ifndef SIMDUTF_IMPLEMENTATION_AVX512BW
#define SIMDUTF_IMPLEMENTATION_AVX512BW (SIMDUTF_IS_X86_64)
#endif

#if SIMDUTF_IMPLEMENTATION_AVX512BW

namespace simdutf {
namespace avx512bw {
} // namespace avx512bw
} // namespace simdutf

#include "simdutf/avx512bw/implementation.h"

#include "simdutf/avx512bw/begin.h"
#include "simdutf/avx512bw/end.h"

#endif // SIMDUTF_IMPLEMENTATION_AVX512BW
#endif // SIMDUTF_AVX512BW_H
