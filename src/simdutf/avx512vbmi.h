#ifndef SIMDUTF_AVX512VBMI_H
#define SIMDUTF_AVX512VBMI_H

#include "simdutf/portability.h"

#ifndef SIMDUTF_IMPLEMENTATION_AVX512VBMI
#define SIMDUTF_IMPLEMENTATION_AVX512VBMI (SIMDUTF_HAS_AVX512F && \
                                           SIMDUTF_HAS_AVX512DQ && \
                                           SIMDUTF_HAS_AVX512VL && \
                                           SIMDUTF_HAS_AVX512VBMI)
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
