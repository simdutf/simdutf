#include <stdbool.h>
#include <stdint.h>
#include <uchar.h>

#include "utf16fix.h"

static inline void
utf16fix_inner(char16_t *out, const char16_t *in, size_t n, bool inplace)
{
	size_t i;
	char16_t curr;
	bool prev_hi = false, curr_lo;

	if (n == 0)
		return;

	curr = in[0] & 0xfc00;
	out[0] = curr == 0xdc00 ? 0xfffd : in[0];
	prev_hi = curr == 0xd800;

	for (i = 1; i < n; i++) {
		curr = in[i] & 0xfc00;
		curr_lo = curr == 0xdc00;

		/* mismatch? */
		if (prev_hi ^ curr_lo) {
			bool prev_ill, curr_ill;

			prev_ill = prev_hi & !curr_lo;
			curr_ill = !prev_hi & curr_lo;

			out[i - 1] = prev_ill ? 0xfffd : out[i - 1];
			out[i] = curr_ill ? 0xfffd : in[i];
		} else if (!inplace)
			out[i] = in[i];

		prev_hi = curr == 0xd800;
	}

	/* string may not end with high surrogate */
	if (prev_hi)
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
