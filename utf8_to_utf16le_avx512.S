	.section	.rodata
	.balign		64
	// identity permutation
.Lid:	.byte		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
	.byte		16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31
	.byte		32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47
	.byte		48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63

.L80:	.byte		0x80, 0x80, 0x80, 0x80
.Lc0:	.byte		0xc0, 0xc0, 0xc0, 0xc0
.Le0:	.byte		0xe0, 0xe0, 0xe0, 0xf0
.Lf0:	.byte		0xf0, 0xf0, 0xf0, 0xf0
.Lc2:	.byte		0xc2, 0xc2, 0xc2, 0xc2

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
	vpbroadcastd	.Lc2(%rip), %zmm20

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

	vpcmpltub	%zmm20, %zmm0, %k5	// zmm0 < 0xc2
	ktestq		%k2, %k5		// 0xc0 <= zmm0 < 0xc2 (illegal two byte sequence?)
	jnz		.Lfail

	// move masks into scalar registers to play with
	kmovq		%k1, %r11
	kmovq		%k2, %r12
	kmovq		%k3, %r13
	kmovq		%k4, %r14

	// compute the various bit masks we need to assembly the UTF-16 characters
	// of note: we will later only process Unicode characters that start no later
	// than 5 bytes before the end of the input.  LAST may contain wrong data for
	// the last 4 bytes of input, but we don't care about that a whole lot.
	andn		%r11, %r12, %r8		// FOLLOW: follow bytes
	andn		%r13, %r14, %r10	// LEAD3: lead bytes for 3 byte sequences
	mov		%r8, %rcx
	not		%rcx			// LEAD: UTF-8 first bytes
	lea		(%r14, %r10, 1), %r9	// LEAD34: lead bytes for 3 or 4 byte sequences


	// check if there is an encoding error
	// check for correct character sequencing and validity
	// we don't need to check for 0xf8 <= zmm0 here: if this
	// occurs, we decode it as a 4 byte sequence which will
	// encode a character > U+10FFFF and is rejected later on
	add		%r12, %r12		// where follow bytes directly after a lead byte should be
	shl		$2, %r13		// where follow bytes two bytes after a lead byte should be
	shl		$3, %r14		// where follow bytes three bytes after a lead byte should be
	or		%r14, %r13
	or		%r13, %r11		// where we expect follow bytes to be
	cmp		%r11, %r8		// does that match reality?
	jne		.Lfail			// if not, I've got bad news for you

	or		%r14, %rcx		// FIRST+4TH: the first and the fourth byte of each sequence
	add		%rcx, %rcx		// LASR+3RD: the last byte of each sequence and the third byte of each
	kmovq		%rcx, %k6		// ... 4 byte sequence, i.e. where we want to start to decode the input

	vpcompressb	%zmm27, %zmm7{%k6}{z}	// the indices of the input bytes marked in L+3RD
	vmovzxbw	%ymm7, %zmm7		// ... zero expanded into words

	// join the bits of each UTF-16 word and load the last bytes
	vpmovdqu8	%zmm30, %zmm6{%k1}{z}	// ASCII: 00000000  other: 11000000
	vpandnd		%zmm0, %zmm6, %zmm0	// high two bits cleared where not ASCII
	vpermb		%zmm0, %zmm7, %zmm1{%k7}{z} // the last byte of each character

	// add in second last bytes
	and		%r14, %r8		// LAST+3RD bits that are not first bits
	shr		$1, %r8			// those bits preceeding them (unless ASCII)
	kmovq		%r8, %k6

	vpaddw		%zmm26, %zmm7, %zmm7	// indices of the second last bytes
	vmovdqu8	%zmm0, %zmm6{%k6}{z}	// only byte that are the second last byte of a sequence
	vpermb		%zmm6, %zmm7, %zmm2{%k7}{z} // the second last bytes (of two, three byte seq, surrogates)
	vpsllw		$6, %zmm2, %zmm2	// shifted into position
	vpaddw		%zmm2, %zmm1, %zmm1	// and add to the last bytes

	// add in third last bytes
	kmovq		%r9, %k6		// LEAD34 -> those bytes we want to add in
	vpaddw		%zmm26, %zmm7, %zmm7	// indices of the third last bytes
	vmovdqu8	%zmm0, %zmm6{%k6}{z}	// only those that are the third last byte of a sequece
	vpermb		%zmm6, %zmm7, %zmm3{%k7}{z} // the third last bytes (of three byte sequences, hi surrogate)
	vpsllw		$12, %zmm3, %zmm3	// shifted into position
	vpaddw		%zmm3, %zmm1, %zmm1	// and added to the other bytes

	// now we have the following situation; the tag bits have already been cleared.
	// --- CASE ---  -------------- INPUT --------------  ---- OUTPUT ----
	// ascii                                    0GFEDCBA  000000000GFEDCBA
	// 2 byte                          110LKJHG 10FEDCBA  00000LKJHGFEDCBA
	// 3 byte                 1110RQPN 10MLKJHG 10FEDCBA  RQPNMLKJHGFEDCBA
	// hi surrogate  11110WVU 10TSRQPN 10MLKJHG           0WVUTSRQPNMLKJHG
	// lo surrogate                    10MLKJHG 10FEDCBA  0000MLKJHGFEDCVA

	pext		%rcx, %r14, %r13	// words in zmm1 that represent high surrogates
	kmovd		%r13d, %k1
	add		%r13d, %r13d		// words in zmm1 that represent low surrogates
	kmovd		%r13d, %k2
	kord		%k1, %k2, %k3		// words in zmm1 that represent surrogates

	// post process surrogates.  Here we assume that each high surrogate is followed by a low surrogate
	vmovdqu16	%zmm25, %zmm6{%k2}{z}	// lo surr: 1111110000000000  other: 00000000 00000000
	vpexpandw	%zmm24, %zmm7{%k3}{z}	// hi surr: 1101011111000000  lo surr: 1101110000000000
	vpslrw		$4, %zmm1, %zmm1{%k1}	// hi surr: 00000WVUTSRQPNML, others unchanged
	vpandnd		%zmm1, %zmm6, %zmm1	// lo surr: 000000KJHGFEDCBA, others unchanged
	vpaddw		%zmm7, %zmm1, %zmm1     // hi:    110110vutsRQPNML  lo surr: 110111VUTSRQPNML
						// vuts = WVUTS - 1

	// check 3 and 4 byte sequences for correctness
	shl		$3, %r10		// the end bytes of 3 byte sequences
	pext		%rcx, %r10, %r10	// words in zmm1 that represent three byte sequences
	kmovd		%r10d, %k4
	vpcmpltuw	%zmm23, %zmm1, %k4{%k4}	// any 3 byte characters < 0x0800?

	vpsubw		%zmm22, %zmm1, %zmm2	// zmm1 - 0xd800
	knotd		%k3, %k6
	vpcmpltuw	%zmm23, %zmm2, %k5{%k6}	// any unexpected surrogates?
	kord		%k4, %k5, %k5
	vpcmpnltuw	%zmm24, %zmm2, %k4{%k1}	// any botched high surrogates?  These occur when
						// char < U+10000 or > U+10FFFF or lead byte >F7
	kortestd	%k4, %k5
	jnz		.Lfail

	// compute the number characters in zmm1, depost, and iterate
	vmovdqu16	%zmm1, (%rdi)
	mov		$-1, %edx
	pdep		%rcx, %rdx, %rcx	// the first up to 32 bit in %rcx
	lzcnt		%rcx, %rax
	mov		$1, %edx
	ror		%rax, %rdx		// the bit after the highest bit in rcx
	and		%r14, %rdx		// does this bit correspond to a high surrogate?
	andn		%rcx, %rdx, %rcx	// if it does, clear it in RCX (last word must not be high surrogate)

	.size utf8_to_utf16le_avx512, .-utf8_to_utf16le_avx512
