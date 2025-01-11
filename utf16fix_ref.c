#include <stdbool.h>
#include <stdint.h>
#if defined(__APPLE__)
typedef uint16_t char16_t;
#else
 #include <uchar.h>
#endif

#include "utf16fix.h"

/* reference implementation */
void
utf16fix_reference(char16_t *out, const char16_t *in, size_t n)
{
	size_t i;
	bool high_surrogate_prev = false, high_surrogate, low_surrogate;

	for (i = 0; i < n; i++) {
		high_surrogate = is_high_surrogate(in[i]);
		low_surrogate = is_low_surrogate(in[i]);

		if (high_surrogate_prev && !low_surrogate)
			out[i - 1] = 0xfffd;

		if (!high_surrogate_prev && low_surrogate)
			out[i] = 0xfffd;
		else
			out[i] = in[i];

		high_surrogate_prev = high_surrogate;
	}

	/* string may not end with high surrogate */
	if (high_surrogate_prev)
		out[i - 1] = 0xfffd;
}

