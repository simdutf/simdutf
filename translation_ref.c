#include <uchar.h>

/*
 * Translate the input from UTF-16LE to UTF-8 until the buffer ends or
 * an encoding error occured.  Return the number of words translated and
 * write the length of the output to outlen.  Note that the output
 * buffer length is not checked.  It must be long enough to hold the
 * translated string.  If a surrogate character is split over the end of
 * the buffer, the last character may not be translated.  If this
 * occurs, read more input into the buffer and continue translation.
 */
extern size_t
utf16le_to_utf8_ref(unsigned char out[restrict], const char16_t in[restrict], size_t len, size_t *outlen)
{
	size_t inpos = 0, rem = len, outpos = 0;

	while (rem > 0) {
		unsigned c0 = in[inpos];

		if (c0 < 0x0080) { /* ASCII */
			rem -= 1;
			inpos++;
			out[outpos++] = c0;
		} else if (c0 < 0x0800) { /* two byte */
			rem -= 1;
			inpos++;
			out[outpos++] = c0 >> 6 | 0xc0;
			out[outpos++] = c0 & 0x3f | 0x80;
		} else if (0xd800 <= c0 && c0 < 0xdc00) { /* high surrogate */
			unsigned c1;
			unsigned long c;

			if (rem < 2)
				break; /* high surrogate at end of buffer */

			c1 = in[inpos + 1];
			if (c1 < 0xdc00 || c1 >= 0xe000)
				break; /* high surrogate not followed by low surrogate */

			c = c1 + (c0 << 10) - 0xdc00 - (0xd800 << 10) + 0x10000;
			rem -= 2;
			inpos += 2;
			out[outpos++] = c >> 18        | 0xf0;
			out[outpos++] = c >> 12 & 0x3f | 0x80;
			out[outpos++] = c >>  6 & 0x3f | 0x80;
			out[outpos++] = c       & 0x3f | 0x80;
		} else if (0xdc00 <= c0 && c0 < 0xe000) /* low surrogate */
			break; /* unmatched low surrogate */
		else { /* 3 byte */
			rem -= 1;
			inpos++;
			out[outpos++] = c0 >> 12        | 0xe0;
			out[outpos++] = c0 >>  6 & 0x3f | 0x80;
			out[outpos++] = c0       & 0x3f | 0x80;
		}
	}

	*outlen = outpos;
	return (len - rem);
}

/*
 * Compute the buffer length needed to translate input of n UTF-16LE
 * characters into UTF-8.  This buffer space may vary slightly with
 * implementation due to operating in chunks.
 */
extern size_t
utf16le_to_utf8_buflen_ref(size_t n)
{
	return (4*n);
}
