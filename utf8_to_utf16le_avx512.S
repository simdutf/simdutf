	.section	.rodata
	.balign		64
	// identity permutation
.Lid:	.byte		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
	.byte		16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31
	.byte		32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47
	.byte		48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63

.L55:	.quad		0x5555555555555555

.L80:	.byte		0x80, 0x80, 0x80, 0x80
.Lc0:	.byte		0xc0, 0xc0, 0xc0, 0xc0
.Le0:	.byte		0xe0, 0xe0, 0xe0, 0xf0
.Lf0:	.byte		0xf0, 0xf0, 0xf0, 0xf0

.Lfc00:	.short		0xfc00fc00
.Ld7c0dc00:
	.short		0xd800 - 0x0400, 0xdc00
.L0800:	.short		0x0800, 0x0800
.Ld800:	.short		0xd800, 0xd800
.L0400:	.short		0x0400, 0x0400

	.text
	.globl		utf8_to_utf16le_avx512
	.type		utf8_to_utf16le_avx512, @function
	.balign		16
utf8_to_utf16le_avx512:
	// rdi: outbuf
	// rsi: inbuf
	// rdx: inlen
	// rcx: &outlen
	vpbroadcastd	.L80(%rip), %zmm31
	vpbroadcastd	.Lc0(%rip), %zmm30
	vpbroadcastd	.Le0(%rip), %zmm29
	vpbroadcastd	.Lf0(%rip), %zmm28

	vmovdqa8	.Lid(%rip), %zmm27
	vpternlogd	$0xff, %zmm26, %zmm26, %zmm26 // ffffffff
	vpbroadcastd	.Lfc00(%rip), %zmm25
	vpbroadcastd	.Ld7c0dc00(%rip), %zmm24
	vpbroadcastd	.L0800(%rip), %zmm23
	vpbroadcastd	.Ld800(%rip), %zmm22
	vpbroadcastd	.L0400(%rip), %zmm21

	mov		%rdi, %r8		// stash output buffer for latter
	mov		%rsi, %r9		// dito with input buffer

	mov		$0x5555555555555555, %rax
	kmovq		%rax, %k7		// the LSB of each word

	cmp		$64, %rdx		// enough left for another iteration?
	jb		.Ltail

.Loop:	vmovdqu8	(%rsi), %zmm0		// load 64 byte from input buffer
	vpcmpnltub	%zmm31, %zmm0, %k1	// 0x80 <= zmm0 (not ASCII)
	ktestq		%k1, %k1		// all ASCII?
	jnz		1f

	// Fast path 1: all ASCII characters
	vextracti32x8	$1, %zmm0, %ymm2
	vpmovzxbw	%ymm0, %zmm1		// convert to UTF-16LE
	vpmovzxbw	%ymm2, %zmm2
	vpmovdqu16	%zmm1, (%rdi)		// and deposit in output buffer
	vpmovdqu16	%zmm2, 64(%rdi)

	add		$64, %rsi		// advance source buffer
	add		$128, %rdi		// advance output buffer
	sub		$64, %rdx		// apply processed input
	cmp		$64, %rdx		// enough left for another iteration?
	jae		.Loop			// if yes, go again!

	// classify characters further
1:	vpcmpnltub	%zmm30, %zmm0, %k2	// 0xc0 <= zmm0 (2, 3, or 4 start bytes)
	vpcmpnltub	%zmm29, %zmm0, %k3	// 0xe0 <= zmm0 (3 or 4 byte start bytes)
	vpcmpnltub	%zmm28, %zmm0, %k4	// 0xf0 <= zmm0 (4 byte start bytes)

	// check for correct character sequencing and validity
	// we don't need to check for 0xf8 <= zmm0 here: if this
	// occurs, we decode it as a 4 byte sequence which will
	// encode a character > U+10FFFF and is rejected later on

	kmovq		%k1, %r11		// move masks into scalar registers to play with
	kmovq		%k2, %r12
	kmovq		%k3, %r13
	kmovq		%k4, %r14

	kandn		%r11, %r12, %r8		// indices at which follow bytes are located
	mov		%r8, %rax
	not		%rax			// indices at which lead bytes are located
	test		$1, %al			// does our input begin with a lead byte?
	jz		.Lfail			// if not, we cannot decode anything.

	shr		%rax			// indices at which last bytes are located
	lea		(%rax, %r14, 8), %rax	// and indices at which the third bytes of 4 byte sequences are located
	kmovq		%rax, %k6
	vpcompressb	%zm27, %zmm7{%k6}	// the locations of these bytes compressed
	vpmovzxbw	%ymm7, %zmm7		// and zero expanded into works

	mov		$-1, %r10d		// 32 set bits
	pdep		%r9, %r10, %r10		// indices in zmm0 at which the characters begin that
						// will end up being decoded into the output.  For
						// surrogates, two slots are allocated: one holds the
						// first two bytes, the other the second two bytes


	// produce a vector of all elements that must be follow bytes according to the first bytes
	shl		$1, %r12		// indices that must hold the second byte of a sequence
	shl		$2, %r13		// indices that must hold the third byte of a sequence
	shl		$3, %r14		// indices that must hold the fourth byte of a sequence
	or		%r12, %r13
	or		%r13, %r14		// indices that must hold a follow byte
	cmp		%r14, %r8		// are these the same at which follow bytes are actually located?
	jne		.Lfail			// if not, we have a problem, houston.

	// join the bits of each UTF-16 word
	vpmovdqu8	%zmm30, %zmm6{%k1}{z}	// ASCII: 00000000  other: 11000000
	vpandnd		%zmm0, %zmm6, %zmm0	// high two bits cleared where not ASCII
	vpermb		%zmm0, %zmm7, %zmm1{%k7}{z} // the last byte of each character

	vpaddw		%zmm27, %zmm7, %zmm7	// indices of the second last bytes
	vmovdqu8	%zmm0, %zmm6{...}{z}	// only byte that are the second byte of a sequence
	vpermb		%zmm6, %zmm7, %zmm2{%k7}{z} // the second last bytes (of two, three byte seq, surrogates)
	vpsllw		$6, %zmm2, %zmm2	// shifted into position
	vpaddw		%zmm2, %zmm1, %zmm1	// and add to the last bytes

	vpaddw		%zmm27, %zmm7, %zmm7	// indices of the third last bytes
	vmovdqu8	%zmm0, %zmm6{...}{z}	// only those that are the third last byte of a sequece
	vpermb		%zmm6, %zmm7, %zmm3{%k7}{z} // the third last bytes (of three byte sequences, surrogates)
	vpsllw		$12, %zmm3, %zmm3	// shifted into position
	vpaddw		%zmm3, %zmm1, %zmm1	// and added to the other bytes

	// now we have the following situation; the tag bits have already been cleared.
	// --- CASE ---  -------------- INPUT --------------  ---- OUTPUT ----
	// ascii                                    0GFEDCBA  000000000GFEDCBA
	// 2 byte                          110LKJHG 10FEDCBA  00000LKJHGFEDCBA
	// 3 byte                 1110RQPN 10MLKJHG 10FEDCBA  RQPNMLKJHGFEDCBA
	// hi surrogate  11110WVU 10TSRQPN 10MLKJHG           0WVUTSRQPNMLKJHG
	// lo surrogate           10TSRQPN 10MLKJHG 10FEDCBA  RQPNMLKJHGFEDCVA

	// post process surrogates.  Here we assume that each high surrogate is followed by a low surrogate
	vmovdqu16	%zmm25, %zmm6{...}{z}	// lo surr: 1111110000000000  other: 00000000 00000000
	vpexpandw	%zmm24, %zmm7{...}{z}	// hi surr: 1101011111000000  lo surr: 1101110000000000
	vpslrw		$4, %zmm1, %zmm1{...}	// hi surr: 00000WVUTSRQPNML, others unchanged
	vpandnd		%zmm1, %zmm6, %zmm1	// lo surr: 000000KJHGFEDCBA, others unchanged
	vpaddw		%zmm24, %zmm1, %zmm1{...} // hi:    110110vutsRQPNML  lo surr: 110111VUTSRQPNML
						// vuts = WVUTS - 1

	// TODO: also check 2 byte sequences (earlier)
	vpcmpltuw	%zmm23, %zmm1, %k1{...}	// any 3 byte characters < 0x0800?
	vpsubb		%zmm22, %zmm1, %zmm2	// zmm1 - 0xd800
	vpcmpltuw	%zmm23, %zmm2, %k2{...}	// any unexpected surrogates?
	kord		%k1, %k2, %k2
	vpcmpnltuw	%zmm24, %zmm2, %k1{...}	// any botched high surrogates?  These occur when
						// char < U+10000 or > U+10FFFF or lead byte >F7
	kortestd	%k1, %k2
	jnz		.Lfail

	.size utf8_to_utf16le_avx512, .-utf8_to_utf16le_avx512
