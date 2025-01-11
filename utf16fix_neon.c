#ifdef __ARM_NEON

#include <arm_neon.h>

#include "utf16fix.h"

/*
 * Returns if a vector of type uint8x16_t is all zero.
 * Note that this only works reliably for vectors where each
 * byte is either 0 or -1.
 */
static int
veq_zero(uint8x16_t v)
{
#ifdef __arm__
	union { uint8x8_t v; double d; } narrowed;

	/* narrow each byte to a nibble */
	narrowed.v = vshrnnarrowed.v = vshrn_n_u16(vreinterpretq_u16_u8(v), 4);

	return (narrowed.d == 0.0);
#else /* AArch64 */
	uint8x8_t narrowed;

	/* narrow each byte to a nibble */
	narrowed = vshrn_n_u16(vreinterpretq_u16_u8(v), 4);

	/* check if that vector is all zero */
	return (vdupd_lane_f64(vreinterpret_f64_u16(narrowed), 0) == 0.0);
#endif
}

/*
 * Process one block of 16 characters.  If in_place is false,
 * copy the block from in to out.  If there is a sequencing
 * error in the block, overwrite the illsequenced characters
 * with the replacement character.  This function reads one
 * character before the beginning of the buffer as a lookback.
 * If that character is illsequenced, it too is overwritten.
 */
static inline void
utf16fix_block(char16_t *out, const char16_t *in, bool inplace)
{
	uint8x16x2_t lb, block;
	uint8x16_t lb_masked, block_masked, lb_is_high, block_is_low;
	uint8x16_t illseq;

	/* TODO: compute lookback using shifts */
	lb = vld2q_u8((const uint8_t *)(in - 1));
	block = vld2q_u8((const uint8_t *)in);
	lb_masked = vandq_u8(lb.val[1], vdupq_n_u8(0xfc));
	block_masked = vandq_u8(block.val[1], vdupq_n_u8(0xfc));
	lb_is_high = vceqq_u8(lb_masked, vdupq_n_u8(0xd8));
	block_is_low = vceqq_u8(block_masked, vdupq_n_u8(0xdc));

	illseq = veorq_u8(lb_is_high, block_is_low);
	if (!veq_zero(illseq)) {
		uint8x16_t lb_illseq, block_illseq;
		char16_t lb;
		int ill;

		/* compute the cause of the illegal sequencing */
		lb_illseq = vbicq_u8(lb_is_high, block_is_low);
		block_illseq = vorrq_u8(
			vbicq_u8(block_is_low, lb_is_high),
			vextq_u8(lb_illseq, vdupq_n_u8(0), 1));

		/* fix illegal sequencing in the lookback */
		ill = vgetq_lane_u8(lb_illseq, 0);
		lb = out[-1];
		out[-1] = ill ? 0xfffd : lb;

		/* fix illegal sequencing in the main block */
		block.val[0] = vbslq_u8(block_illseq, vdupq_n_u8(0xfd), block.val[0]);
		block.val[1] = vorrq_u8(block_illseq, block.val[1]);

		vst2q_u8((uint8_t *)out, block);
	} else if (!inplace)
		vst2q_u8((uint8_t *)out, block);
}

void
utf16fix_neon(char16_t *out, const char16_t *in, size_t n)
{
	size_t i;

	if (n < 17) {
		utf16fix_generic(out, in, n);
		return;
	}

	out[0] = is_low_surrogate(in[0]) ? 0xfffd : in[0];

	/* duplicate code to have the compiler specialise utf16fix_block() */
	if (in == out) {
		for (i = 1; i + 16 < n; i += 16)
			utf16fix_block(out + i, in + i, true);

		/* tbd: find carry */
		utf16fix_block(out + n - 16, in + n - 16, true);
	} else {
		for (i = 1; i + 16 < n; i += 16)
			utf16fix_block(out + i, in + i, false);

		utf16fix_block(out + n - 16, in + n - 16, false);
	}

	out[n - 1] = is_high_surrogate(out[n - 1]) ? 0xfffd : out[n - 1];
}
#endif /* defined(__ARM_NEON) */
