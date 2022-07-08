	.section .rodata
	.balign 4
.L0080:	.short		0x0080, 0x0080
.L3f3f:	.short		0x3f3f, 0x3f3f
.L80c0:	.short		0x80c0, 0x80c0
.Lfc00:	.short		0xfc00, 0xfc00
.Ld800:	.short		0xd800, 0xd800
.Ldc00:	.short		0xdc00, 0xdc00
.Lfca02400:
	// clear the surrogate tag bits and apply plane shift
	// 0x10000 - 0xdc00 - (0xd800 << 10)
	.int		0xfca02400
.L8080e000:
	.int		0x8080e000
.L808080f0:
	.int		0x808080f0
.L00010101:
	.int		0x00010101

	.balign	8
	// mask for vpmultishiftbq
	// input:  00000000 000WVUTS RQPNMLKJ HGFEDCBA
	// output: HGFEDCBA PNMLKJHG VUTSRQPN 00000WVU
.Lms:	.byte		18, 12, 6, 0,  50, 44, 38, 32

/*
 * General approach:
 *
 * 1. fast path for all ASCII/all 2 byte character (< U+0800)
 * 2. extend to dword, decode surrogates if needed
 * 3. check for mismatched surrogates if needed
 * 4. shift bits into place using .Lms(%rip)
 * 5. mask and apply Unicode tag bits (0x80, 0xc0, 0xe0, 0xf0)
 *    to the bytes of each dword according to whether it's 2, 3,
 *    or 4 byte.  ASCII bytes skip this step.
 * 6. compress by removing unused bytes
 * 7. deposit into output buffer
 *
 * The tail is handled by padding it with NUL bytes and
 * subtracting these from the output count afterwards.
 */

	.text
	.globl		utf16le_to_utf8_avx512
	.type		utf16le_to_utf8_avx512, @function
	.balign		16
utf16le_to_utf8_avx512:
	// rdi: outbuf
	// rsi: inbuf
	// rdx: inlen
	// rcx: &outlen

	vpbroadcastd	.L0080(%rip), %zmm31
	vpslld		$4, %zmm31, %zmm30	// 0x0800
	vpbroadcastd	.L3f3f(%rip), %zmm29
	vpbroadcastd	.L80c0(%rip), %zmm28

	vpbroadcastd	.Lfc00(%rip), %zmm27
	vpbroadcastd	.Ld800(%rip), %zmm26
	vpbroadcastd	.Ldc00(%rip), %zmm25

	push		%rbx
	xor		%ebx, %ebx		// low surrogate carry

	mov		$0x7fffffff, %eax	// (this constant is also used after .Ltail)
	kmovd		%eax, %k1		// mask ignoring the lookahead

	mov		%rdi, %r8		// stash output buffer for later
	mov		%rsi, %r9		// same with input buffer
	xor		%eax, %eax		// number by which to adjust output length

	cmp		$32, %rdx		// enough left for a zmm register?
	jb		.Ltail			// if not, tail processing time!

	.balign		16
.Loop:	vmovdqu16	(%rsi), %zmm0
	sub		$31, %rdx		// account for the bytes consumed
.Llastiteration:
	add		$62, %rsi		// and advance to next block (sans lookahead)

.Lfailiteration:
	vpcmpnltuw	%zmm31, %zmm0, %k2{%k1}	// 0x0080 <= zmm0? except lookahead (non-ASCII character?)
	ktestd		%k2, %k2		// if any non-ASCII character is present,
	jnz		1f			// do slower processing, else ...

	// fast path 1: all ASCII characters
	vpmovwb		%zmm0, (%rdi)		// compress into ASCII and deposit

	add		$31, %rdi		// advance output buffer
	xor		%ebx, %ebx		// clear surrogate carry
	cmp		$32, %rdx		// enough left for another round?
	jae		.Loop			// if yes, go again!
	jmp		.Ltail			// if not, tail processing time!

1:	vpcmpltuw	%zmm30, %zmm0, %k3	// zmm0 < 0x0800? (1--2 byte character?)
	ktestd		%k1, %k3		// if any >2 byte characters are present,
	jnc		2f			// do slower processing, else ...

	// fast path 2: all 1 and 3 byte encodings
	// input:  00000LKJ HGFEDCBA or 00000000 0GFEDCBA
	// output: 10FEDCBA 110LKJHG or 0GFEDCBA
	vpsrlw		$6, %zmm0, %zmm1	// 00000000 000LKJHG
	vpsllw		$8, %zmm0, %zmm2	// HGFEDCBA 00000000
	vpternlogd	$0xa8, %zmm29, %zmm2, %zmm1 // 00FEDCBA 000LKJHG  zmm1 = (zmm1|zmm2) & 0x3f3f
	vpaddw		%zmm28, %zmm1, %zmm0{%k2} // 10FEDCBA 110LKJHG or 00000000 0GFEDCBA

	kmovd		%k2, %r10d		// determine advance based on k2
	popcnt		%r10d, %r10d		// convert to number of two byte characters

	vpcmpnltub	%zmm30, %zmm0, %k3	// 01 if ASCII, 11 if 2 byte
	vpcompressb	%zmm0, %zmm0{%k3}{z}	// smoosh all characters together
	vmovdqu8	%zmm0, (%rdi)		// and deposit into memory

	lea		31(%rdi, %r10, 1), %rdi	// advance destination buffer
	xor		%ebx, %ebx		// clear surrogate carry
	cmp		$32, %rdx		// enough left for another round?
	jae		.Loop			// if yes, go again!
	jmp		.Ltail			// if not, tail processing time!

2:	vpbroadcastq	.Lms(%rip), %zmm23	// multishift mask
	vpbroadcastd	.L8080e000(%rip), %zmm16 // 3 byte character tag bits
	vmovdqa32	%zmm16, %zmm17		// same for high characters

	vextracti32x8	$1, %zmm0, %ymm2
	vpmovzxwd	%ymm0, %zmm1		// low 16 characters as dwords
	vpmovzxwd	%ymm2, %zmm2		// high 15 characters as dwords

	kxord		%k6, %k6, %k6		// surrogate carry out if no surrogates present

	vpandd		%zmm27, %zmm0, %zmm3	// zmm0 & 0xfc00 (prepare to check for surrogates)
	vpcmpequw	%zmm26, %zmm3, %k4{%k1}	// 0xd800 <= zmm0 < 0xdc00 (high surrogate, except lookahead)
	vpcmpequw	%zmm25, %zmm3, %k5	// 0xdc00 <= zmm0 < 0xe000 (low surrogate)
	kortestd	%k4, %k5		// any surrogates present?
	jz		3f			// if not, skip surrogate preprocessing

	vpbroadcastd	.L808080f0(%rip), %zmm22
	vpbroadcastd	.Lfca02400(%rip), %zmm24

	valignd		$1, %zmm1, %zmm2, %zmm3	// low surrogates corresponding to zmm1
	valignd		$1, %zmm2, %zmm1, %zmm4	// low surrogates corresponding to zmm2

	// check for mismatched surrogates
	kmovd		%k4, %r10d		// move hi/lo surrogate masks to general purpose registers
	kmovd		%k5, %r11d		// this moves the validation off p0/p5, making it no longer
						// part of the critical path

	lea		(%rbx, %r10, 2), %ebx	// high surrogates shifted in place of low surrogates w/ carry
	andn		%r11d, %ebx, %ebx	// low surrogate without high surrogate before?
	shr		%r11d			// low surrogates shifted in place of high surrogates
	andn		%r10d, %r11d, %r11d	// high surrogate without low surrogate after?
	or		%r11d, %ebx		// either of these two cases?
	jnz		.Lfail			// if yes, encoding error, else we have ebx == 0

	// preprocess surrogates in zmm1, zmm2
	// input hi: 110110VU TSRQPNML  lo: 110111KJ HGFEDCBA
	// wvuts = 0VUTS + 1 (plane shift)
	// output: 00000000 000wvuts RQPNMLKJ HGFEDCBA
	kshiftrd	$16, %k4, %k6		// location of high surrogates in zmm2
	vmovdqa32	%zmm22, %zmm16{%k4}	// 3 or 4 character tag bits for zmm1
	vmovdqa32	%zmm22, %zmm17{%k6}	// ... and zmm2

	vpslld		$10, %zmm1, %zmm1{%k4}	// if 4 byte: 00000011 0110VUTS RQPNML00 00000000
	vpslld		$10, %zmm2, %zmm2{%k6}
	vpaddd		%zmm24, %zmm3, %zmm3	//            11111100 10100001 000000KJ HGFEDCBA
	vpaddd		%zmm24, %zmm4, %zmm4
	vpaddd		%zmm3, %zmm1, %zmm1{%k4} //if 4 byte: 00000000 000wvuts RQPNMLKJ HGFEDCBA
	vpaddd		%zmm4, %zmm2, %zmm2{%k6}

	// compute masks to ignore low surrogates in compress step
	// if no surrogates occur, k4 and k5 are conveniently already clear
	kshiftrd	$16, %k5, %k4		// which of zmm2 correspond to low surrogates?
	vpmovm2d	%k5, %zmm3		// ffffffff if low surrogate, 00000000 otherwise
	vpmovm2d	%k4, %zmm4		// same for zmm2
	vpmovb2m	%zmm3, %k4		// which bytes in zmm1 correspond to low surrogates?
	vpmovb2m	%zmm4, %k5		// which bytes in zmm2 correspond to low surrogates?

	shr		$30, %r10d		// set up !ZF == high surrogate carry out

	// transcode 1, 2, 3, and 4 byte characters at once!
3:	vpmultishiftqb	%zmm1, %zmm23, %zmm3	// HGFEDCBA PNMLKJHG VUTSRQPN 00000WVU
	vpmultishiftqb	%zmm2, %zmm23, %zmm4

	vpbroadcastd	.L00010101(%rip), %zmm21
	setnz		%bl			// set up surrogate carry out

	knotd		%k2, %k2		// which of zmm2:zmm1 are ASCII characters?
	kshiftrd	$16, %k3, %k7		// which of zmm2 are 1 or 2 byte characters?
	kshiftrd	$16, %k2, %k6		// which of zmm2 are ASCII characters?
	vpslld		$16, %zmm28, %zmm16{%k3} // 2 byte: 80c00000  3 byte: 8080e000  4 byte: 808080f0
	vpslld		$16, %zmm28, %zmm17{%K7} // same for high characters
	vpternlogd	$0xea, %zmm16, %zmm29, %zmm3 // zmm1 = zmm1 & 0x3f3f3f3f | zmm16
	vpternlogd	$0xea, %zmm17, %zmm29, %zmm4 // this masks and applies tag bits in one step.
	vpslld		$24, %zmm1, %zmm3{%k2}	// if ASCII, 0GFEDCBA 00000000 00000000 00000000
	vpslld		$24, %zmm2, %zmm4{%k6}

	// Okay, kinda confusing, isn't it?  Here's what is going on.
	// CASE    INPUT IN ZMM1/ZMM2       ZMM16/17  OUTPUT IN ZMM1/ZMM2
	// 1 byte  00000 00000000 0GFEDCBA  --------  0GFEDCBA 00000000 00000000 00000000
	// 2 byte  00000 00000LKJ HGFEDCBA  80c00000  10FEDCBA 110LKJHG 00000000 00000000
	// 3 byte  00000 RQPNMLKJ HGFEDCBA  8080e000  10FEDCBA 10MLKJHG 1110RQPN 00000000
	// 4 byte  wvuts RQPNMLKJ HGFEDCBA  808080f0  10FEDCBA 10MLKJHG 10tsRQPN 11110wvu

	// smoosh characters together and output
	vpcmpnltub	%zmm21, %zmm3, %k2	// which bytes in zmm1 do we want to deposit?
	vpcmpnltub	%zmm21, %zmm4, %k3	// (all nonzero bytes and the MSB of each dword)
	kandnq		%k2, %k4, %k2		// do not deposit bytes corresponding to low surrogates
	kandnq		%k3, %k5, %k3
	vpcompressb	%zmm3, %zmm3{%k2}{z}	// smoosh characters together
	vpcompressb	%zmm4, %zmm4{%k3}{z}

	kmovq		%k2, %r10
	kmovq		%k3, %r11
	popcnt		%r10, %r10		// number of UTF-8 bytes in zmm1
	shl		$4, %r11		// ignore lookahead in zmm2
	popcnt		%r11, %r11		// number of UTF-8 bytes in zmm2

	vmovdqu8	%zmm3, (%rdi)		// deposit first half of output
	vmovdqu8	%zmm4, (%rdi, %r10, 1)	// deposit second half of output
	add		%r10, %rdi		// advance past first half of output
	add		%r11, %rdi		// advance past second half of output

	cmp		$32, %rdx		// enough left for another round?
	jae		.Loop			// if yes, go again!

.Ltail:	test		%edx, %edx		// anything left to decode at all?
	jnz		4f			// if yes, process it

.Lend:	add		%rax, %rdi		// subtract padding characters from buffer end
	sub		%r9, %rsi		// input length = buffer end - buffer start
	sar		$1, %rsi		// in characters
	add		%rsi, %rax		// ... - number of padding characters
	sub		%r8, %rdi		// output length = buffer end - buffer start
	mov		%rdi, (%rcx)		// deposit into *outlen

	pop		%rbx
	vzeroupper
	ret

	// Process <31 words tail.
	// Strategie: pad remaining input with U+0000 and move r8 back to account
	// for the size of the padding
4:	mov		$0x7fffffff, %eax
	bzhi		%edx, %eax, %eax	// eax = 0x7fffffff & (1 << edx) - 1
	kmovd		%eax, %k7		// mask of words still in the buffer
	vmovdqu16	(%rsi), %zmm0{%k7}{z}	// manually load tail padded with U+000000
	lea		-31(%rdx), %rax		// set up r10 with the neg. number of padding characters
	xor		%edx, %edx		// no further input to be processed
	jmp		.Llastiteration

	// decoding failure
.Lfail:	tzcnt		%ebx, %edx		// where did the decoding error occur?
	kmovd		%k5, %ebx		// fake the surrogate carry: if an error occured due to
	and		$1, %ebx		// the carry not matching a leading low surrogate, the
						// final iteration would produce zero bytes anyway

	// Dock tail to length of its valid prefix
	// this is very similar to the tail processing code
	mov		$0x7fffffff, %eax
	bzhi		%edx, %eax, %eax	// eax = 0x7fffffff & (1 << edx) - 1
	kmovd		%eax, %k7		// mask of words still in the buffer
	vmovdqu16	%zmm0, %zmm0{%k7}{z}	// mask input to valid prefix padded with U+000000
	lea		-31(%rdx), %rax		// do not count NUL byte padding in output length
	xor		%edx, %edx		// no further input to be processed
	jmp		.Lfailiteration

	.size		utf16le_to_utf8_avx512, .-utf16le_to_utf8_avx512


// return the length of the output buffer for transcoding the given number of characters
	.type		utf16le_to_utf8_buflen_avx512, @function
	.globl		utf16le_to_utf8_buflen_avx512
utf16le_to_utf8_buflen_avx512:
	lea		95(,%rdi,4), %rax	// up to 4 bytes for each character
						// plus 95 byte we might store past the end
	ret

	.size		utf16le_to_utf8_buflen_avx512, .-utf16le_to_utf8_buflen_avx512
