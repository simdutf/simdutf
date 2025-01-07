#include <stddef.h>

#include "utf16fix.h"

struct utf16fix_impl utf16fix_impls[] = {
	"reference", utf16fix_reference,
	"generic", utf16fix_generic,
#ifdef __SSE2__
	"sse", utf16fix_sse,
#endif
	NULL, NULL,
};
