#ifdef __SSE2__
#include <stdbool.h>
#include <stdint.h>
#include <emmintrin.h>
#include <uchar.h>

#include "utf16fix.h"

/* for big-endian operation, byte-swap all masks */

/*
 * Process one block of 8 characters.  If in_place is false,
 * copy the block from in to out.  If there is a sequencing
 * error in the block, overwrite the illsequenced characters
 * with the replacement character.  This function reads one
 * character before the beginning of the buffer as a lookback.
 * If that character is illsequenced, it too is overwritten.
 */
static void
utf16fix_block(char16_t *out, const char16_t *in, bool in_place)
{
	__m128i lookback, block, lb_masked, block_masked, lb_is_high, block_is_low;
	__m128i illseq, lb_illseq, block_illseq;

	lookback = _mm_loadu_si128((const void *)(in - 1));
	block = _mm_loadu_si128((const void *)in);

	lb_masked = _mm_and_si128(lookback, _mm_set1_epi16(0xfc00));
	block_masked = _mm_and_si128(block, _mm_set1_epi16(0xfc00));
	lb_is_high = _mm_cmpeq_epi16(lb_masked, _mm_set1_epi16(0xd800));
	block_is_low = _mm_cmpeq_epi16(block_masked, _mm_set1_epi16(0xdc00));

	illseq = _mm_xor_si128(lb_is_high, block_is_low);
	if (_mm_movemask_epi8(illseq) != 0) {
		int lb;

		/* compute the cause of the illegal sequencing */
		lb_illseq = _mm_andnot_si128(block_is_low, lb_is_high);
		block_illseq = _mm_or_si128(
			_mm_andnot_si128(lb_is_high, block_is_low),
			_mm_bsrli_si128(lb_illseq, 2));

		/* fix illegal sequencing in the lookback */
		lb = _mm_cvtsi128_si32(lb_illseq);
		lb = lb & 0xfffd | ~lb & out[-1];
		out[-1] = lb & 0xffff;

		/* fix illegal sequencing in the main block */
		block = _mm_or_si128(
			_mm_andnot_si128(block_illseq, block),
			_mm_and_si128(block_illseq, _mm_set1_epi16(0xfffd)));

		_mm_storeu_si128((void *)out, block);
	} else if (!in_place)
		_mm_storeu_si128((void *)out, block);
}

void
utf16fix_sse(char16_t *out, const char16_t *in, size_t n)
{
	size_t i;
	bool in_place;

	if (n < 9) {
		utf16fix_generic(out, in, n);
		return;
	}

	out[0] = is_low_surrogate(in[0]) ? 0xfffd : in[0];

	/* duplicate code to have the compiler specialise utf16fix_block() */
	if (in == out) {
		for (i = 1; i + 8 < n; i += 8)
			utf16fix_block(out + i, in + i, true);

		utf16fix_block(out + n - 8, in + n - 8, true);
	} else {
		for (i = 1; i + 8 < n; i += 8)
			utf16fix_block(out + i, in + i, false);

		utf16fix_block(out + n - 8, in + n - 8, false);
	}

	out[n - 1] = is_high_surrogate(out[n - 1]) ? 0xfffd : out[n - 1];
}
#endif
