	.extern	utf8_to_utf16le_neon_tab

	.section .rodata
	.balign	16
magic:
	// lower bounds by class
	.short	0x0000, 0x0080, 0x0800, 0xffff, 0xffff, 0xd800, 0xdc00, 0xffff
	// upper bounds by class
	.short  0x007f, 0x07ff, 0xffff, 0x0000, 0x0000, 0xdbff, 0xdfff, 0x0000
	// high tetranacci numbers
	.short	108, 208, 401, 773, 1490, 2872, 5536, 10671
	// low tetranacci numbers
	.byte	0, 1, 2, 4,  8, 15, 29, 56

	.text
	.type	utf8_to_utf16le_avx512, @function
	.globl	utf8_to_utf16le_avx512
	// size_t utf8_to_utf16le_neon(char16_t *outbuf, const char8_t *inbuf, size_t inlen, size_t *outlen)
utf8_to_utf16le_neon:
outbuf	.req	x0
inbuf	.req	x1
inlen	.req	x2
outlen	.req	x3

	adrp	x4, magic
	add	x4, x4, #:lo12:magic
	ldr	q31, [x4, #32]			// high Tetranacci numbers
	ldr	d30, [x4, #48]			// low Tetranacci numbers

	adrp	x5, utf8_to_utf16le_neon_tab	// lookup table for shuffles
	add	x5, x5, #:lo12:utf8_to_utf16le_neon_tab

	subs	inlen, inlen, #16		// can we process at least 16 bytes?
	blo	.Lrunt

	/* main loop */
.Lloop:	ldr	q0, [inbuf]			// load a chunk of input
	ldrb	w16, [inbuf]			// load first byte of chunk

	// classify q0 into LEAD and FOLLOW bytes and compute Tetranacci index
	movi	v1.16b, #2
	ushr	v2.16b, v0.16b, #6		// inbuf >> 6
	cmeq	v1.16b, v1.16b, v2.16b		// (inbuf >> 6) == 2, that is 0x80 <= inbuf < 0xc0 (FOLLOW)
	sxtl2	v2.8h, v1.16b
	and	v3.8b, v1.8b, v30.8b		// Tetranacci numbers for low half
	and	v2.16b, v2.16b, v21.16b		// Tetranacci numbers for high half
	uaddw	v2.8h, v2.8h, v3.8b		// for both halves
	uaddlv	s2, v2.8h			// Tetranacci index
	fmov	w15, s2

	// ensure the input starts with a lead-byte and has no more than
	// 3 consecutive follow bytes
	and	w16, w16, #0xc0
	cmp	w16, #0x80			// 0x80 <= inbuf[0] < 0xc0 (FOLLOW)?
	beq	.Lerror0			// if inbuf[0] is FOLLOW, error out

						// (rotation behaves like shift as inbuf[0] is never FOLLOW)
	ext	v1.16b, v0.16b, v0.16b, #1	// inbuf rotated right one slot
	and	v1.16b, v1.16b, v0.16b		// 0xff where two consecutive FOLLOW bytes
	ext	v2.16b, v1.16b, v1.16b, #2	// v1.16b rotated right two slots
	and	v1.16b, v1.16b, v2.16b		// 0xff where four consecutive FOLLOW bytes
	umaxv	s2, v1.4s			// nonzero if four consecutive FOLLOW bytes anywhere
	fmov	w16, s2
	cbnz	w16, .Lerror1			// if four consecutive FOLLOW bytes, error out

	// load from lookup table and determine conversion length
	ldr	d2, [x5, x15, lsl #3]		// load table entry
	add	x15, x5, x15, lsl #3
	ldrsb	w15, [x15, #7]			// last byte of table entry
	movi	v1.8b, #0x8f
	and	v1.8b, v1.8b, v2.8b		// source indices or negative number if none
	smaxv	b1, v1.8b			// last index transcoded
	fmov	w14, s1
	add	w14, w14, #1			// number of input bytes transcoded
	mov	w16, #8				// w15 non-negative -> 8 halfwords of output
	tst	w15, #0x80			// w15 negative -> -w15 halfwords of output
	csneg	w15, w16, w15, eq		// w15 = w15 > 0 ? 8 : -w15 -> output size in halfwords

	// load last three bytes of each sequence (or possibly junk)
	sxtl	v3.8h, v2.8b			// LUT entry as halfwords
	movi	v1.8h, #1
	bic	v3.8h, #0x70			// clear tag bits
	tbl	v4.16b, {v0.16b}, v3.16b	// load last byte of each sequence
	sub	v3.8h, v3.8h, v1.8h
	tbl	v5.16b, {v0.16b}, v3.16b	// load penultimate byte of each sequence
	sub	v3.8h, v3.8h, v1.8h
	tbl	v6.16b, {v0.16b}, v3.16b	// load antepenultimate byte of each sequence

.Lrunt:	udf	#0

	// first byte if input is FOLLOW byte
.Lerror0:
	udf	#0

	// four consecutive FOLLOW bytes in input
.Lerror1:
	udf	#0

	.size	utf8_to_utf16le_neon, .-utf8_to_utf16le_neon
