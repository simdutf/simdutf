	.extern	utf8_to_utf16le_neon_tab

	.section .rodata
	.balign	16
magic:
	/*
	 * Bounds Checks
	 *
	 * After assembling the three bytes of every sequence into a halfword
	 * according to the formula C0 + (C1 << 6) + (C2 << 12) without first
	 * stripping off tag bits, we check if the LEAD byte is correct for
	 * the indicated case.  As we are certain about the other bytes being
	 * FOLLOW bytes, this can be done with a simple range check, which
	 * then also catches overly long encodings.  We do the range check on
	 * a halfword holding (C0 + (C1 << 6) + (C2 << 12)) >> 4, as the low
	 * nibble never matters.
	 *
	 * The cases are (cf mktab_neon.c):
	 *
	 * CASE   RANGE   (C0 C1 C2)  RANGE (MASK)
	 * ASCII  00 00 00--00 00 7F  00000--0007F
	 * 2BYTE  00 C2 80--00 DF BF  03100--0387F
	 * 3BYTE  E0 A0 80--EF BF BF  E2880--F207F
	 * 4HEAD  F0 90 80--F4 8F BF  F2480--F647F
	 * 4TAIL  00 80 80--00 BF BF  02080--0307F
	 *
	 * The end of each range is inclusive.
	 * Unused entries are set to 0xffff for easier debugging.
	 * The upper bound is stored as the difference between the lower and
	 * the upper bound, allowing us to turn a comparison of the form
	 *
	 *     low <= x <= hi
	 *
	 * into one of the form
	 *
	 *     x - low <= hi
	 */
	.short 0x0000, 0x0310, 0xe288, 0xffff, 0xffff, 0x0208, 0xf248, 0xffff
	.short 0x0007-0x0000, 0x0387-0x0310, 0xf207-0xe288, 0xffff, 0xffff, 0x0307-0x0208, 0xf647-0xf248, 0xffff

	/*
	 * Tag Bit Removal
	 *
	 * We remove tag bits by subtracting the right tag bit mask from the
	 * joined sequence.  As we are dealing with halfwords, the upper four
	 * bits are ignored, but listed here for completeness.  The cases are:
	 *
	 * CASE   TAG BITS  MASK
	 * ASCII  00 00 00  00000
	 * 2BYTE  00 C0 80  03080
	 * 3BYTE  E0 80 80  E2080
	 * 4HEAD  F0 80 80  FA080
	 * 4TAIL  00 80 80  02080
	 *
	 * In the 4HEAD case, the mask is shifted four places to the right to
	 * account for the input being shifted.  We add 0040 to the mask, which translates the
	 * codepoint from the Unicode range to the high surrogate range by
	 * subtracting 10000.  We also subtract D800, to apply the high
	 * surrogate mask.  Likewise, we subtract the low surrogate mask DC00
	 * in the 4TAIL case.
	 */
	.short 0x0000, 0x3080, 0x2080, 0xffff, 0xffff, 0x2480, 0x1a48, 0xffff

	/*
	 * Surrogate preprocessing
	 *
	 * The 4BYTE cases need two special steps to be done before the tag bit
	 * removal step can insert the surrogate masks D800 and DC00 (for 4HEAD
	 * and 4TAIL respectively):
	 *
	 *  - 4HEAD entries need to be shifted 4 bytes to the right
	 *  - 4TAIL entries need to have the M and L bits cleared
	 *
	 * We achieve both in one step by selecting from C0+(C1<<6)+(C2<<12) and
	 * the same shifted to the right by four bits.  For 4HEAD we take the
	 * shifted-right variant.  For 4TAIL we take the shifted-right variant
	 * for the top six bits, which will always be zero (thus masking out the
	 * M and L bits, as well as the tag bits of C1).  For other cases we
	 * always select the unshifted value.  The following table has the
	 * appropriate masks.
	 */
	.short 0x0000, 0x0000, 0x0000, 0xffff, 0xffff, 0xfc00, 0xffff, 0xffff

	/*
	 * Tetranacci Indices
	 *
	 * We index into utf8_to_utf16le_neon_tab using Tetranacci numbers,
	 * which uniquely assigns one index to each FOLLOW syndrome with
	 * three or less consecutive FOLLOW bytes.  The first byte is assumed
	 * to be of LEAD type; if it is not, the sequencing validation will
	 * fail at a later point.  See mktab_neon.c for more details.
	 */
	// high tetranacci numbers
	.short	108, 208, 401, 773, 1490, 2872, 5536, 10671
	// low tetranacci numbers
	.byte	0, 1, 2, 4,  8, 15, 29, 56

	.text
	.type	utf8_to_utf16le_neon, @function
	.globl	utf8_to_utf16le_neon
	// size_t utf8_to_utf16le_neon(char16_t *outbuf, const char8_t *inbuf, size_t inlen, size_t *outlen)
utf8_to_utf16le_neon:
outbuf	.req	x0
inbuf	.req	x1
inlen	.req	x2
outlen	.req	x3

	adrp	x4, magic
	add	x4, x4, #:lo12:magic
	ldp	q28, q29, [x4, #0]		// range for bounds checks
	ldp	q27, q26, [x4, #32]		// tag bit mask, selection mask
	ldr	q31, [x4, #64]			// high Tetranacci numbers
	ldr	d30, [x4, #80]			// low Tetranacci numbers

	adrp	x5, utf8_to_utf16le_neon_tab	// lookup table for shuffles
	add	x5, x5, #:lo12:utf8_to_utf16le_neon_tab

	mov	x12, inbuf			// original input pointer
	mov	x13, outbuf			// original output pointer

	subs	inlen, inlen, #16		// can we process at least 16 bytes?
	blo	.Lrunt

	/* main loop */
.Lloop:	ldr	q0, [inbuf]			// load a chunk of input

	// classify q0 into LEAD and FOLLOW bytes
.Lloop1:
	movi	v1.16b, #2
	ushr	v2.16b, v0.16b, #6		// inbuf >> 6
	cmeq	v1.16b, v1.16b, v2.16b		// (inbuf >> 6) == 2, that is 0x80 <= inbuf < 0xc0 (FOLLOW)

	// compute the Tetranacci index
	sxtl2	v2.8h, v1.16b
	and	v3.8b, v1.8b, v30.8b		// Tetranacci numbers for low half
	and	v2.16b, v2.16b, v31.16b		// Tetranacci numbers for high half
	uaddw	v2.8h, v2.8h, v3.8b		// for both halves
	uaddlv	s2, v2.8h			// Tetranacci index

	shrn	v6.8b, v1.8h, #4		// convert FOLLOW syndrome to nibble mask
	fmov	x16, d6
	fmov	w11, s2
	cbz	x16, .Lascii			// all ASCII?

.Lnotascii:
	// ensure the input has no more than 3 consecutive follow bytes
	and	x17, x16, x16, lsr #4		// consecutive pairs of set nibbles?
//	beq	...				// if none, go to ASCII/2BYTE fast path
	tst	x17, x17, lsr #8		// consecutive quadruples of set nibbles?
	bne	.Lerror_4follow			// if any, we have an encoding error

	// load from lookup table and determine conversion length
	ldr	d2, [x5, x11, lsl #3]		// load table entry
	add	x11, x5, x11, lsl #3
	ldrsb	w15, [x11, #7]			// last byte of table entry
//	prfm	pldl1keep, [x11, #16]

	movi	v3.8b, #0x70
	bic	v3.8b, v2.8b, v3.8b		// LUT entry without tag bits (but with invalid bit)

	// load last three bytes of each sequence (or possibly junk)
	movi	v1.8b, #1
	tbl	v4.8b, {v0.16b}, v3.8b		// load last byte of each sequence (C0)
	sub	v6.8b, v3.8b, v1.8b
	tbl	v5.8b, {v0.16b}, v6.8b		// load penultimate byte of each sequence (C1)
	sub	v6.8b, v6.8b, v1.8b
	tbl	v6.8b, {v0.16b}, v6.8b		// load antepenultimate byte of each sequence (C2)

	smaxv	b1, v3.8b			// last index transcoded
	fmov	w14, s1
	add	w14, w14, #1			// number of input bytes transcoded
	mov	w10, #8				// w15 non-negative -> 8 halfwords of output
	tst	w15, #0x80			// w15 negative -> -w15 halfwords of output
	csneg	w15, w10, w15, eq		// w15 = w15 > 0 ? 8 : -w15 -> output size in halfwords

	/*
	 * We now have for each case:
	 * CASE   C2 (V6)   C1 (V5)   C0 (V4)
	 * ASCII  ????????  ????????  0GFEDCBA
	 * 2BYTE  ????????  110LKJHG  10FEDCBA
	 * 3BYTE  1110RQPN  10MLKJHG  10FEDCBA
	 * 4HEAD  11110WVU  10TSRQPN  10MLKJHG
	 * 4TAIL  10TSRQPN  10MLKJHG  10FEDCBA
	 * EMPTY  00000000  00000000  00000000
	 */

	// strip out bytes before the start of each sequence
	movi	v1.8b, #0x30
	movi	v7.8b, #0x20
	cmtst	v1.8b, v1.8b, v2.8b		// 2BYTE, 3BYTE, 4HEAD, 4TAIL
	cmtst	v7.8b, v7.8b, v2.8b		// 3BYTE, 4HEAD
	and	v5.8b, v5.8b, v1.8b		// clear penultimate byte in ASCII
	and	v6.8b, v6.8b, v7.8b		// clear antepenultimate byte in ASCII/2BYTE/4TAIL

	/*
	 * We now have for each case:
	 * CASE   C2 (V6)   C1 (V5)   C0 (V4)
	 * ASCII  00000000  00000000  0GFEDCBA
	 * 2BYTE  00000000  110LKJHG  10FEDCBA
	 * 3BYTE  1110RQPN  10MLKJHG  10FEDCBA
	 * 4HEAD  11110WVU  10TSRQPN  10MLKJHG
	 * 4TAIL  00000000  10MLKJHG  10FEDCBA
	 * EMPTY  00000000  00000000  00000000
	 */

	// assemble bytes with tag bits intact into v4 and v5 (shifted 4 places right)
	ushll	v5.8h, v5.8b, #6		// C1 <<  6 as halfword
	shll	v6.8h, v6.8b, #8		// C2 <<  8 as halfword
	uaddw	v4.8h, v5.8h, v4.8b		// C0 + (C1 << 6) as halfwords
	shl	v7.8h, v6.8h, #4		// C2 << 12 as halfword
	ushr	v5.8h, v4.8h, #4		// (C0 + (C1 << 6)) >> 4
	add	v4.8h, v4.8h, v7.8h		// C0 + (C1 << 6) + (C2 << 12)
	add	v5.8h, v5.8h, v6.8h		// (C0 + (C1 << 6) + (C2 << 12)) >> 4

	/*
	 * We now have for each case:
	 * CASE   V4                                 V5
	 * ASCII  000000000GFEDCBA+0000000000000000  0000000000000GFE+0000000000000000
	 * 2BYTE  00000LKJHGFEDCBA+0011000010000000  000000000LKJHGFE+0000001100001000
	 * 3BYTE  RQPNMLKJHGFEDCBA+0010000010000000  0000RQPNMLKJHGFE+1110001000001000
	 * 4HEAD  0WVUTSRQPNMLKJHG+0010000010000000  00000WVUTSRQPNML+1111001000001000
	 * 4TAIL  0000MLKJHGFEDCBA+0010000010000000  00000000MLKJHGFE+0000001000001000
	 */

	// compute bounds check and tag masks and strip tag bits
	sshr	v6.8b, v2.8b, #4		// only the tag bits (incl. the invalid bit)
	zip1	v6.16b, v6.16b, v6.16b		// each byte doubled up
	movi	v3.8h, #0x01, lsl #8		// = 0x0100
	sli	v3.16b, v6.16b, #1		// tag bits as half-word indices
	tbl	v2.16b, {v26.16b}, v3.16b	// selection mask
	tbl	v1.16b, {v27.16b}, v3.16b	// UTF-8 tag bit mask
	tbl	v6.16b, {v28.16b}, v3.16b	// lower bounds check mask
	tbl	v7.16b, {v29.16b}, v3.16b	// upper bounds check mask
	bit	v4.16b, v5.16b, v2.16b		// v4 = v5&v2 | v4&~v2

	/*
	 * We now have for each case:
	 * CASE   V4
	 * ASCII  000000000GFEDCBA+0000000000000000
	 * 2BYTE  00000LKJHGFEDCBA+0011000010000000
	 * 3BYTE  RQPNMLKJHGFEDCBA+0010000010000000
	 * 4HEAD  00000WVUTSRQPNML+1111001000001000
	 * 4TAIL  000000KJHGFEDCBA+0010000010000000
	 */

	sub	v4.8h, v4.8h, v1.8h		// strip tag bits, apply surrogate masks

	/*
	 * We now have for each case:
	 * CASE   V4
	 * ASCII  000000000GFEDCBA
	 * 2BYTE  00000LKJHGFEDCBA
	 * 3BYTE  RQPNMLKJHGFEDCBA
	 * 4HEAD  110110vutsRQPNML
	 * 4TAIL  110111KJHGFEDCBA
	 */

	// check for bound violations
	sub	v6.8h, v5.8h, v6.8h		// (char>>4) - lower_bound
	cmhi	v6.8h, v6.8h, v7.8h		// (char>>4) - lower_bound > upper_bound

	// check for stray surrogates
	movi	v2.8h, #0xd8, lsl #8		// = 0xd800, high surrogate tag
	movi	v7.8h, #0x08, lsl #8		// = 0x0800, length of surrogate area
	movi	v1.16b, #0x08
	sub	v0.8h, v4.8h, v2.8h		// v4.8h - 0xd800
	cmhi	v0.8h, v7.8h, v0.8h		// v4.8h - 0xd800 < 0x800 -> 0xd800 <= v4.8h < 0xe000
	cmtst	v1.16b, v1.16b, v3.16b		// FFFF where 4HEAD, 4TAIL, or EMPTY, 0000 otherwise
	bic	v0.16b, v0.16b, v1.16b		// ignore surrogates where we expect them

	// final validation
1:	orr	v6.16b, v6.16b, v0.16b		// stray surrogate or out of range?
	xtn	v6.8b, v6.8h			// compress syndrome to 64 bits
	fmov	x6, d6
	cbnz	x6, .Lerror_range_check		// if any, encoding error

	str	q4, [outbuf]			// write out transcoded data
	add	inbuf, inbuf, x14		// advance input buffer
	add	outbuf, outbuf, x15, lsl #1	// advance outbuf buffer
	subs	inlen, inlen, x14		// deduct processed input from length
	bhs	.Lloop				// and go again if we can!

	// We leave the main loop with some bytes of input remaining.
	// Pass these to utf8_to_utf16le_ref for further processing.
	// TODO: figure out how to process the tail in parallel.
.Lrunt:	add	inlen, inlen, #16		// undo initial subtraction
.Ltail:	sub	x12, inbuf, x12			// number of bytes processed so far
	sub	x13, outbuf, x13		// number of bytes written out so far

	stp	outlen, lr, [sp, #-16]!
	stp	x12, x13, [sp, #-16]!		// save context
	bl	utf8_to_utf16le_ref		// process tail with scalar kernel
	ldp	x12, x13, [sp], #16		// restore context
	ldp	outlen, lr, [sp], #16

	add	x0, x0, x12			// number of input bytes processed in total
	ldr	x1, [outlen]
	add	x1, x1, x13, lsr #1		// nuber of halfwords written out in total
	str	x1, [outlen]
	ret

	// ASCII-only fast path
.Lascii:
	cmlt	v6.16b, v0.16b, #0		// any non-ASCII characters in the input?
	shrn	v6.8b, v6.8h, #4		// convert to nibble mask
	fmov	x17, d6
	cbnz	x17, .Lnotascii			// if non-ASCII detected, return to normal path

	movi	v1.16b, #0
1:	add	inbuf, inbuf, #16		// advance to next chunk of input
	st2	{v0.16b, v1.16b}, [outbuf], #32	// zero-extend and deposit
	subs	inlen, inlen, #16		// consume next chunk of input
	blo	.Lrunt				// until end of input encountered

	// assume the following data is ASCII, too
	ld1	{v0.16b}, [inbuf]		// load next chunk of input
	cmlt	v6.16b, v0.16b, #0		// any non-ASCII characters in the input?
	shrn	v6.8b, v6.8h, #4
	fmov	x17, d6
	cbnz	x17, .Lloop1			// if not, go back to main path
	b	1b

	// 4 or more FOLLOW bytes in a row
.Lerror_4follow:
	and	x17, x17, x17, lsr #8		// where are the 4 consecutive follow bytes?
	rbit	x17, x17			// in reverse order
	clz	x17, x17			// number of bits before the error
	mov	inlen, #3			// proceed until third follow byte
	add	inlen, inlen, x17, lsr #2
	b	.Ltail

	// stray surrogate or range check failed
	// try again with input until the failing sequence sans its final byte
.Lerror_range_check:
	rbit	x6, x6
	clz	x6, x6				// location of first error, in bits
	lsr	x6, x6, #3			// location of first error, in bytes
	ldrb	w6, [x11, x6]			// last byte of the errorneous sequence
	and	inlen, x6, #0xf			// without tags bits
	and	x6, x6, #0xf0			// just the tag bits
	cmp	x6, #0x60			// is it 4HEAD?
	cinc	inlen, inlen, eq		// if yes, remove end of 4TAIL, not 4HEAD
	b	.Ltail
	.size	utf8_to_utf16le_neon, .-utf8_to_utf16le_neon

	.type	utf8_to_utf16le_buflen_neon, @function
	.globl	utf8_to_utf16le_buflen_neon
utf8_to_utf16le_buflen_neon:
	ret
	.size	utf8_to_utf16le_buflen_neon, .-utf8_to_utf16le_buflen_neon
