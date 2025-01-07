#include <stdbool.h>
#include <stdint.h>
#include <uchar.h>

#include "utf16fix.h"

static inline void
utf16fix_inner(char16_t *out, const char16_t *in, size_t n, bool inplace)
{
	size_t i;
	char16_t prev, curr;

	if (n == 0)
		return;

	curr = in[0] & 0xfc00;
	out[0] = curr == 0xdc00 ? 0xfffd : in[0];
	prev = curr;

	for (i = 1; i < n; i++) {
		curr = in[i] & 0xfc00;

		/* mismatch? */
		if (prev == 0xd800 ^ curr == 0xdc00) {
			char16_t a, b;

			a = out[i - 1];
			b = in[i];
			out[i - 1] = curr == 0xdc00 ? a : 0xfffd;
			out[i] = curr == 0xdc00 ? 0xfffd : b;
		} else if (!inplace)
			out[i] = in[i];

		prev = curr;
	}

	/* string may not end with high surrogate */
	if (prev == 0xd800)
		out[i - 1] = 0xfffd;
}

void
utf16fix_generic(char16_t *out, const char16_t *in, size_t n)
{
	if (in == out)
		utf16fix_inner(out, in, n, true);
	else
		utf16fix_inner(out, in, n, false);
}
