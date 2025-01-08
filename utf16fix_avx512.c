#ifdef __AVX512BW__
#include <stdbool.h>
#include <stdint.h>
#include <immintrin.h>
#include <uchar.h>

#include "utf16fix.h"

/* for big-endian operation, byte-swap all masks */

/*
 * Process one block of 32 characters.  If in_place is false,
 * copy the block from in to out.  If there is a sequencing
 * error in the block, overwrite the illsequenced characters
 * with the replacement character.  This function reads one
 * character before the beginning of the buffer as a lookback.
 * If that character is illsequenced, it too is overwritten.
 */
static void
utf16fix_block(char16_t *out, const char16_t *in, bool in_place)
{
	__m512i lookback, block, lb_masked, block_masked;
	__mmask32 lb_is_high, block_is_low, illseq;

	lookback = _mm512_loadu_si512((const void *)(in - 1));
	block = _mm512_loadu_si512((const void *)in);
	lb_masked = _mm512_and_epi32(lookback, _mm512_set1_epi16(0xfc00));
	block_masked = _mm512_and_epi32(block, _mm512_set1_epi16(0xfc00));

	lb_is_high = _mm512_cmpeq_epi16_mask(lb_masked, _mm512_set1_epi16(0xd800));
	block_is_low = _mm512_cmpeq_epi16_mask(block_masked, _mm512_set1_epi16(0xdc00));
	illseq = _kxor_mask32(lb_is_high, block_is_low);
	if (!_ktestz_mask32_u8(illseq, illseq)) {
		__mmask32 lb_illseq, block_illseq;

		/* compute the cause of the illegal sequencing */
		lb_illseq = _kandn_mask32(block_is_low, lb_is_high);
		block_illseq = _kor_mask32(
			_kandn_mask32(lb_is_high, block_is_low),
			_kshiftri_mask32(lb_illseq, 1));

		/* fix illegal sequencing in the lookback */
		lb_illseq = _kand_mask32(lb_illseq, _cvtu32_mask32(1));
		_mm512_mask_storeu_epi16(out - 1, lb_illseq, _mm512_set1_epi16(0xfffd));

		/* fix illegal sequencing in the main block */
		if (in_place)
			_mm512_mask_storeu_epi16(out, block_illseq, _mm512_set1_epi16(0xfffd));
		else
			_mm512_storeu_epi32(out,
				_mm512_mask_blend_epi16(block_illseq, block, _mm512_set1_epi16(0xfffd)));
	} else if (!in_place)
		_mm512_storeu_si512((void *)out, block);
}

void
utf16fix_avx512(char16_t *out, const char16_t *in, size_t n)
{
	size_t i;

	if (n < 33) {
		utf16fix_generic(out, in, n);
		return;
	}

	out[0] = is_low_surrogate(in[0]) ? 0xfffd : in[0];

	/* duplicate code to have the compiler specialise utf16fix_block() */
	if (in == out) {
		for (i = 1; i + 32 < n; i += 32)
			utf16fix_block(out + i, in + i, true);

		utf16fix_block(out + n - 32, in + n - 32, true);
	} else {
		for (i = 1; i + 32 < n; i += 32)
			utf16fix_block(out + i, in + i, false);

		utf16fix_block(out + n - 32, in + n - 32, false);
	}

	out[n - 1] = is_high_surrogate(out[n - 1]) ? 0xfffd : out[n - 1];
}
#endif /* defined(__AVX2__) */
