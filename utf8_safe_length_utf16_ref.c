#include <uchar.h>

/*
 * From the given UTF-8 encoded buffer, compute the number of char16_t
 * needed to represent the contents of the buffer up to its first error
 * or, if entirely valid, its end.  Return the number of bytes translated
 * and write the number of words put out to outlen.
 */
extern size_t
utf8_safe_length_utf16_ref(const unsigned char in[restrict], size_t len, size_t *outlen)
{
	size_t inpos = 0, rem = len, outpos = 0;

	while (rem > 0) {
		unsigned c0 = in[inpos];

		if (c0 < 0x80) { /* ASCII */
			rem--;
			inpos++;
			outpos++;
		} else if (0xc0 <= c0 && c0 < 0xe0) {
			unsigned c1, c;

			if (rem < 2)
				break; /* truncated input */

			c1 = in[inpos + 1];
			if (c1 < 0x80 || c1 >= 0xc0)
				break; /* not a follow byte */

			c = (c0 << 6) + c1 - (0xc0 << 6) - 0x80;
			if (c < 0x80)
				break; /* non-canonical encoding */

			rem -= 2;
			inpos += 2;
			outpos++;
		} else if (0xe0 <= c0 && c0 < 0xf0) {
			unsigned c1, c2, c;

			if (rem < 3)
				break; /* truncated input */

			c1 = in[inpos + 1];
			c2 = in[inpos + 2];
			if (c1 < 0x80 || c1 >= 0xc0)
				break; /* not a follow byte */

			if (c2 < 0x80 || c2 >= 0xc0)
				break; /* not a follow byte */

			c = (c0 << 12) + (c1 << 6) + c2 - (0xe0 << 12) - (0x80 << 6) - 0x80;
			if (c < 0x800)
				break; /* non-canonical encoding */

			if (0xd800 <= c && c < 0xe000)
				break; /* character encodes surrogate */

			rem -= 3;
			inpos += 3;
			outpos++;
		} else if (0xf0 <= c0 && c0 < 0xf8) {
			unsigned c1, c2, c3;
			unsigned long c;

			if (rem < 4)
				break; /* truncated input */

			c1 = in[inpos + 1];
			c2 = in[inpos + 2];
			c3 = in[inpos + 3];
			if (c1 < 0x80 || c1 >= 0xc0)
				break; /* not a follow byte */

			if (c2 < 0x80 || c2 >= 0xc0)
				break; /* not a follow byte */

			if (c3 < 0x80 || c3 >= 0xc0)
				break; /* not a follow byte */

			c = (c0 << 18) + (c1 << 12) + c3 - (0xf0 << 18) - (0x80 << 12) - (0x80 << 6) - 0x80;
			if (c < 0x010000)
				break; /* non-canonical encoding */

			if (c > 0x10ffff)
				break; /* not a Unicode character */

			rem -= 4;
			inpos += 4;
			outpos += 2;
		} else
			break; /* encoding error */
	}

	*outlen = outpos;

	return (len - rem);
}
