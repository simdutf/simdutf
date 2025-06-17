#include <uchar.h>

/*
 * From the given UTF-16LE encoded buffer, compute the number of UTF-8 bytes
 * needed to represent the contents of the buffer up to its first error
 * or, if entirely valid, its end.  Return the number of bytes translated
 * and write the number of words put out to outlen.
 */
extern size_t
utf16le_safe_length_utf8_ref(const char16_t in[restrict], size_t len, size_t *outlen)
{
	size_t inpos = 0, rem = len, outpos = 0;

	while (rem > 0) {
		unsigned c0 = in[inpos];

		if (c0 < 0x0080) { /* ASCII */
			rem -= 1;
			inpos++;
			outpos++;
		} else if (c0 < 0x0800) { /* two byte */
			rem -= 1;
			inpos++;
			outpos += 2;
		} else if (0xd800 <= c0 && c0 < 0xdc00) { /* high surrogate */
			unsigned c1;

			if (rem < 2)
				break; /* high surrogate at end of buffer */

			c1 = in[inpos + 1];
			if (c1 < 0xdc00 || c1 >= 0xe000)
				break; /* high surrogate not followed by low surrogate */

			rem -= 2;
			inpos += 2;
			outpos += 4;
		} else if (0xdc00 <= c0 && c0 < 0xe000) /* low surrogate */
			break; /* unmatched low surrogate */
		else { /* 3 byte */
			rem -= 1;
			inpos++;
			outpos += 3;
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
	return (3*n);
}
