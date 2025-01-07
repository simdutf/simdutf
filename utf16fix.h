#include <stdbool.h>
#include <uchar.h>

/*
 * Copy UTF-16 encoded text in the native byte order from the second
 * argument to the first argument, replacing illegally sequenced
 * surrogates with the replacement character U+FFFD.  The output
 * buffer must either be disjoint from the input buffer or identical,
 * in which case the function operates in place.  Behaviour is
 * undefined if the two buffers overlap but are not identical.
 */
typedef void utf16fix(char16_t *, const char16_t *, size_t);

utf16fix utf16fix_generic, utf16fix_reference;
#ifdef __SSE2__
utf16fix utf16fix_sse;
#endif

extern struct utf16fix_impl {
	const char	*name;
	utf16fix	*impl;
} utf16fix_impls[];

static inline bool
is_high_surrogate(char16_t c)
{
	return (0xd800 <= c && c < 0xdc00);
}

static inline bool
is_low_surrogate(char16_t c)
{
	return (0xdc00 <= c && c < 0xe000);
}
