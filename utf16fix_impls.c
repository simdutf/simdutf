#include <stddef.h>

#include "utf16fix.h"

struct utf16fix_impl utf16fix_impls[] = {
	"reference", utf16fix_reference,
	"generic", utf16fix_generic,
#ifdef __AVX2__
	"avx", utf16fix_avx,
#endif
#ifdef __AVX512BW__
	"avx512", utf16fix_avx512,
#endif
#ifdef __ARM_NEON
	"neon", utf16fix_neon,
	"neon64bits", utf16fix_neon_64bits,

#endif
#ifdef __SSE2__
	"sse", utf16fix_sse,
#endif
	NULL, NULL,
};
