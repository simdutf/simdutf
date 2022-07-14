#include <uchar.h>

/*
 * Check if the input is valid UTF-16.  Return the number of
 * words until the first error or end of string.  Note that
 * a surrogate character split over the end of the buffer may
 * cause the last character to falsely signal as an error.
 * If this occurs, consider reading more input into the buffer
 * and continue validation.
 */
extern size_t
utf16le_validate_ref(const char16_t buf[], size_t len)
{
	size_t pos = 0, rem = len;

	while (rem > 0) {
		unsigned c0 = buf[pos];

		if (c0 < 0xd800) {
			rem -= 1;
			pos += 1;
		} else if (c0 < 0xdc00) {
			unsigned c1;
			unsigned long c;

			/* two word pairs */
			if (rem < 2)
				break; /* surrogate split over end of buffer */

			c1 = buf[pos + 1];
			if (c1 < 0xdc00 || 0xe000 <= c1)
				break; /* unmatched high surrogate */

			rem -= 2;
			pos += 2;
		} else if (c0 < 0xe000)
			break; /* unmatched low surrogate */
		else {
			rem -= 1;
			pos += 1;
		}
	}

	return (len - rem);
}
