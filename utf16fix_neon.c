#ifdef __ARM_NEON

#include <arm_neon.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "utf16fix.h"
/*
 * Returns if a vector of type uint8x16_t is all zero.
 */
static int
veq_non_zero(uint8x16_t v)
{
	// might compile to two instructions:
	//	umaxv   s0, v0.4s
	//	fmov	w0, s0
	// On Apple hardware, they both have a latency of 3 cycles, with a throughput of
	// four instructions per cycle. So that's 6 cycles of latency (!!!) for the two
	// instructions. A narrowing shift has the same latency and throughput.
	return vmaxvq_u32(vreinterpretq_u32_u8(v));
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
	if (veq_non_zero(illseq)) {
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
	i = 1;

	/* duplicate code to have the compiler specialise utf16fix_block() */
	if (in == out) {
		for (; i + 16 < n; i += 16)
			utf16fix_block(out + i, in + i, true);

		/* tbd: find carry */
		utf16fix_block(out + n - 16, in + n - 16, true);
	} else {
		for (; i + 16 < n; i += 16)
			utf16fix_block(out + i, in + i, false);

		utf16fix_block(out + n - 16, in + n - 16, false);
	}

	out[n - 1] = is_high_surrogate(out[n - 1]) ? 0xfffd : out[n - 1];
}


static inline uint8x16_t get_mismatch_copy(const char16_t *in, char16_t *out, bool inplace) {
	uint8x16x2_t lb = vld2q_u8((const uint8_t *)(in - 1));
	uint8x16x2_t block = vld2q_u8((const uint8_t *)in);
	uint8x16_t lb_masked = vandq_u8(lb.val[1], vdupq_n_u8(0xfc));
	uint8x16_t block_masked = vandq_u8(block.val[1], vdupq_n_u8(0xfc));
	uint8x16_t lb_is_high = vceqq_u8(lb_masked, vdupq_n_u8(0xd8));
	uint8x16_t block_is_low = vceqq_u8(block_masked, vdupq_n_u8(0xdc));
	uint8x16_t illseq = veorq_u8(lb_is_high, block_is_low);
	if(!inplace) vst2q_u8((uint8_t *)out, block);
	return illseq;
}
static inline uint64_t get_mask(uint8x16_t illse0, uint8x16_t illse1, uint8x16_t illse2, uint8x16_t illse3) {
	uint8x16_t bit_mask = {0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80,
				0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80};
	uint8x16_t sum0 =
		vpaddq_u8(illse0 & bit_mask, illse1 & bit_mask);
	uint8x16_t sum1 =
		vpaddq_u8(illse2 & bit_mask, illse3 & bit_mask);
	sum0 = vpaddq_u8(sum0, sum1);
	sum0 = vpaddq_u8(sum0, sum0);
	return vgetq_lane_u64(vreinterpretq_u64_u8(sum0), 0);
}

// The idea is to process 64 characters at a time, and if there is a mismatch
// we can fix it with a bit of scalar code. When the input is correct, this function
// might be faster than alternative implementations working on small blocks of input.
static inline bool
utf16fix_block64(char16_t *out, const char16_t *in, bool inplace)
{

	uint8x16_t illse0 = get_mismatch_copy(in, out, inplace);
	uint8x16_t illse1 = get_mismatch_copy(in + 16, out + 16, inplace);
	uint8x16_t illse2 = get_mismatch_copy(in + 32, out + 32, inplace);
	uint8x16_t illse3 = get_mismatch_copy(in + 48, out + 48, inplace);

	// this branch could be marked as unlikely:
	if(veq_non_zero(illse0 | illse1 | illse2 | illse3)) {
		uint64_t matches = get_mask(illse0, illse1, illse2, illse3);
		// Given that ARM has a fast bitreverse instruction, we can
		// reverse once and then use clz to find the first bit set.
		// It is how it is done in simdjson and *might* be beneficial.
		//
		// We might also proceed in reverse to reduce the RAW hazard,
		// but it might require more instructions.
#if defined(__has_builtin) && __has_builtin(__builtin_bitreverse64)
		matches = __builtin_bitreverse64(matches); // rbit
		while (matches != 0) {
			int r = __builtin_clzll(matches); //clz
			bool is_high = is_high_surrogate(in[r-1]);
			bool is_low = is_low_surrogate(in[r]);
			if(is_high && !is_low) {
				out[r - 1] = 0xfffd;
			} else if(!is_high && is_low) {
				out[r] = 0xfffd;
			}
			matches ^= (0x8000000000000000ULL >> r); // lsr + eor
		}
#else
		while (matches != 0) {
			uint64_t t = matches & (~matches + 1);
			int r = __builtin_ctzll(matches); // generates rbit + clz
			bool is_high = is_high_surrogate(in[r-1]);
			bool is_low = is_low_surrogate(in[r]);
			if(is_high && !is_low) {
				out[r - 1] = 0xfffd;
			} else if(!is_high && is_low){
				out[r] = 0xfffd;
			}
			matches ^= t;
		}
#endif
		return false;
	}
	return true;
}


// If we expect the input to have very few mistakes, and often no mistake
// at all, we can optimize the code by checking for mistakes in blocks of 64
// characters at a time. And when mistakes a found, we just need a bit of (slow)
// scalar code to fix them.
void
utf16fix_neon_64bits(char16_t *out, const char16_t *in, size_t n)
{
	size_t i;

	if (n < 17) {
		utf16fix_generic(out, in, n);
		return;
	}

	out[0] = is_low_surrogate(in[0]) ? 0xfffd : in[0];
	i = 1;

	/* duplicate code to have the compiler specialise utf16fix_block() */
	if (in == out) {
		for(i = 1; i + 64 < n; i += 64)
			utf16fix_block64(out + i, in + i, true);

		for (; i + 16 < n; i += 16)
			utf16fix_block(out + i, in + i, true);

		/* tbd: find carry */
		utf16fix_block(out + n - 16, in + n - 16, true);
	} else {
		for(i = 1; i + 64 < n; i += 64)
			utf16fix_block64(out + i, in + i, false);
		for (; i + 16 < n; i += 16)
			utf16fix_block(out + i, in + i, false);

		utf16fix_block(out + n - 16, in + n - 16, false);
	}

	out[n - 1] = is_high_surrogate(out[n - 1]) ? 0xfffd : out[n - 1];
}


static inline bool
utf16fix_block64_simple(char16_t *out, const char16_t *in, bool inplace)
{

	uint8x16_t illse0 = get_mismatch_copy(in, out, inplace);
	uint8x16_t illse1 = get_mismatch_copy(in + 16, out + 16, inplace);
	uint8x16_t illse2 = get_mismatch_copy(in + 32, out + 32, inplace);
	uint8x16_t illse3 = get_mismatch_copy(in + 48, out + 48, inplace);
	uint64_t matches = get_mask(illse0, illse1, illse2, illse3);
	if(matches) {
		while (matches != 0) {
			  uint64_t t = matches & (~matches + 1);
			  int r = __builtin_ctzll(matches); // generates rbit + clz
			  bool is_high = is_high_surrogate(in[r-1]);
			  bool is_low = is_low_surrogate(in[r]);
			  if(is_high && !is_low) {
				out[r - 1] = 0xfffd;
			  } else if(!is_high && is_low){
				out[r] = 0xfffd;
			  }
			  matches ^= t;
		}
		return false;
	}
	return true;
}

void
utf16fix_neon_64bits_simple(char16_t *out, const char16_t *in, size_t n)
{
	size_t i;

	if (n < 17) {
		utf16fix_generic(out, in, n);
		return;
	}

	out[0] = is_low_surrogate(in[0]) ? 0xfffd : in[0];
	i = 1;

	/* duplicate code to have the compiler specialise utf16fix_block() */
	if (in == out) {
		for(i = 1; i + 64 < n; i += 64)
			utf16fix_block64_simple(out + i, in + i, true);

		for (; i + 16 < n; i += 16)
			utf16fix_block(out + i, in + i, true);

		/* tbd: find carry */
		utf16fix_block(out + n - 16, in + n - 16, true);
	} else {
		for(i = 1; i + 64 < n; i += 64)
			utf16fix_block64_simple(out + i, in + i, false);
		for (; i + 16 < n; i += 16)
			utf16fix_block(out + i, in + i, false);

		utf16fix_block(out + n - 16, in + n - 16, false);
	}

	out[n - 1] = is_high_surrogate(out[n - 1]) ? 0xfffd : out[n - 1];
}
#endif /* defined(__ARM_NEON) */
