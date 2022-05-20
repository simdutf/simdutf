#include "simdutf.h"
#include "implementation.cpp"
#include "encoding_types.cpp"
// The large tables should be included once and they
// should not depend on a kernel.
#include "tables/utf8_to_utf16_tables.h"
#include "tables/utf16_to_utf8_tables.h"
// End of tables.

// The scalar routines should be included once.
#include "scalar/utf16_to_utf8/valid_utf16_to_utf8.h"
#include "scalar/utf16_to_utf8/utf16_to_utf8.h"
#include "scalar/utf16_to_utf32/valid_utf16_to_utf32.h"
#include "scalar/utf16_to_utf32/utf16_to_utf32.h"
#include "scalar/utf8_to_utf16/valid_utf8_to_utf16.h"
#include "scalar/utf8_to_utf32/valid_utf8_to_utf32.h"
#include "scalar/utf8_to_utf16/utf8_to_utf16.h"
#include "scalar/utf8_to_utf32/utf8_to_utf32.h"
#include "scalar/utf8.h"
#include "scalar/utf16.h"
//


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
