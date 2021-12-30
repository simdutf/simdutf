#ifndef SIMDUTF_AVX512VBMI_H
#define SIMDUTF_AVX512VBMI_H

#include "simdutf/portability.h"

// An AVX512 implementation can be determined only in runtime.
#ifndef SIMDUTF_IMPLEMENTATION_AVX512VBMI
#define SIMDUTF_IMPLEMENTATION_AVX512VBMI (SIMDUTF_IS_X86_64)
#endif

#if SIMDUTF_IMPLEMENTATION_AVX512VBMI

namespace simdutf {
namespace avx512vbmi {
} // namespace avx512vbmi
} // namespace simdutf

#include "simdutf/avx512vbmi/implementation.h"

#include "simdutf/avx512vbmi/begin.h"
#include "simdutf/avx512vbmi/end.h"

#endif // SIMDUTF_IMPLEMENTATION_AVX512VBMI
#endif // SIMDUTF_AVX512VBMI_H
