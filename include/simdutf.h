#ifndef SIMDUTF_H
#define SIMDUTF_H
#include <string>
#include <cstring>
#include <atomic>
#include <vector>

#include "simdutf/compiler_check.h"
#include "simdutf/common_defs.h"
#include "simdutf/encoding_types.h"

SIMDUTF_PUSH_DISABLE_WARNINGS
SIMDUTF_DISABLE_UNDESIRED_WARNINGS

// Public API
#include "simdutf/simdutf_version.h"
#include "simdutf/implementation.h"


// Implementation-internal files (must be included before the implementations themselves, to keep
// amalgamation working--otherwise, the first time a file is included, it might be put inside the
// #ifdef SIMDUTF_IMPLEMENTATION_ARM64/FALLBACK/etc., which means the other implementations can't
// compile unless that implementation is turned on).
#include "simdutf/internal/isadetection.h"

// Implementations
#include "simdutf/arm64.h"
#include "simdutf/haswell.h"
#include "simdutf/westmere.h"
#include "simdutf/ppc64.h"
#include "simdutf/fallback.h"

SIMDUTF_POP_DISABLE_WARNINGS

#endif // SIMDUTF_H
