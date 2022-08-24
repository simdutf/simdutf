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
	push		%rbx

	vpbroadcastd	.L0080(%rip), %zmm31
	vpslld		$4, %zmm31, %zmm30	// 0x0800
	vpbroadcastd	.L3f3f(%rip), %zmm29
	vpbroadcastd	.L80c0(%rip), %zmm28

	vpbroadcastd	.Lfc00(%rip), %zmm27
	vpbroadcastd	.Ld800(%rip), %zmm26
	vpbroadcastd	.Ldc00(%rip), %zmm25

	vpternlogd	$0xff, %zmm20, %zmm20, %zmm20 // 0xffffffff

	mov		$0x7fffffff, %eax	// (this constant is also used after .Ltail)
	kmovd		%eax, %k1		// mask ignoring the lookahead

	mov		%rdi, %r8		// stash output buffer for later
	push		%rsi			// same with input buffer
	xor		%r9, %r9		// number by which to adjust output length
	xor		%eax, %eax		// low surrogate carry

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
	vpmovwb		%zmm0, (%rdi){%k1}	// compress into ASCII and deposit

	add		$31, %rdi		// advance output buffer
	xor		%eax, %eax		// clear surrogate carry
	cmp		$32, %rdx		// enough left for another round?
	jae		.Loop			// if yes, go again!
	jmp		.Ltail			// if not, tail processing time!

1:	vpcmpltuw	%zmm30, %zmm0, %k3	// zmm0 < 0x0800? (1--2 byte character?)
	ktestd		%k1, %k3		// if any >2 byte characters are present,
	jnc		2f			// do slower processing, else ...

	// fast path 2: all 1 and 3 byte encodings
	// input:  00000LKJ HGFEDCBA or 00000000 0GFEDCBA
	// output: 10FEDCBA 110LKJHG or 0GFEDCBA
	kmovd		%k2, %eax		// determine advance based on k2
	vpsrlw		$6, %zmm0, %zmm1	// 00000000 000LKJHG
	vpsllw		$8, %zmm0, %zmm2	// HGFEDCBA 00000000
	vpternlogd	$0xa8, %zmm29, %zmm2, %zmm1 // 00FEDCBA 000LKJHG  zmm1 = (zmm1|zmm2) & 0x3f3f
	vpaddw		%zmm28, %zmm1, %zmm0{%k2} // 10FEDCBA 110LKJHG or 00000000 0GFEDCBA

	popcnt		%eax, %eax		// convert to number of two byte characters

	vpblendmw	%zmm30, %zmm20, %zmm1{%k1} // 0800 for ASCII/2byte, FFFF for lookahead
	vpcmpnltub	%zmm1, %zmm0, %k3	// 01 if ASCII, 11 if 2 byte, 00 if lookahead
	vpcompressb	%zmm0, %zmm1{%k3}{z}	// smoosh all characters together

	kmovq		%k3, %r10
	pext		%r10, %r10, %r10	// compute mask of valid bytes in zmm1
	kmovq		%r10, %k3
	vmovdqu8	%zmm1, (%rdi){%k3}	// deposit transcoded bytes into output

	lea		31(%rdi, %rax, 1), %rdi	// advance destination buffer
	xor		%eax, %eax		// clear surrogate carry
	cmp		$32, %rdx		// enough left for another round?
	jae		.Loop			// if yes, go again!
	jmp		.Ltail			// if not, tail processing time!

2:	vpbroadcastq	.Lms(%rip), %zmm23	// multishift mask
	vpbroadcastd	.L8080e000(%rip), %zmm16 // 3 byte character tag bits
	vmovdqa32	%zmm16, %zmm17		// same for high characters

	vpandd		%zmm27, %zmm0, %zmm3	// zmm0 & 0xfc00 (prepare to check for surrogates)
	vpcmpequw	%zmm26, %zmm3, %k4{%k1}	// 0xd800 <= zmm0 < 0xdc00 (high surrogate, except lookahead)
	vpcmpequw	%zmm25, %zmm3, %k5	// 0xdc00 <= zmm0 < 0xe000 (low surrogate)

	vextracti32x8	$1, %zmm0, %ymm2
	vpmovzxwd	%ymm0, %zmm1		// low 16 characters as dwords
	vpmovzxwd	%ymm2, %zmm2		// high 15 characters as dwords

	kortestd	%k4, %k5		// any surrogates present?
	jz		3f			// if not, skip surrogate preprocessing

	// preprocess surrogates in zmm1, zmm2
	// input hi: 110110VU TSRQPNML  lo: 110111KJ HGFEDCBA
	// wvuts = 0VUTS + 1 (plane shift)
	// output: 00000000 000wvuts RQPNMLKJ HGFEDCBA
	vpbroadcastd	.L808080f0(%rip), %zmm22
	vpbroadcastd	.Lfca02400(%rip), %zmm24

	valignd		$1, %zmm1, %zmm2, %zmm3	// low surrogates corresponding to zmm1
	valignd		$1, %zmm2, %zmm1, %zmm4	// low surrogates corresponding to zmm2

	kshiftrd	$16, %k4, %k6		// location of high surrogates in zmm2
	vmovdqa32	%zmm22, %zmm16{%k4}	// 3 or 4 character tag bits for zmm1
	vmovdqa32	%zmm22, %zmm17{%k6}	// ... and zmm2

	vpslld		$10, %zmm1, %zmm1{%k4}	// if 4 byte: 00000011 0110VUTS RQPNML00 00000000
	vpslld		$10, %zmm2, %zmm2{%k6}
	vpaddd		%zmm24, %zmm3, %zmm3	//            11111100 10100001 000000KJ HGFEDCBA
	vpaddd		%zmm24, %zmm4, %zmm4
	vpaddd		%zmm3, %zmm1, %zmm1{%k4} //if 4 byte: 00000000 000wvuts RQPNMLKJ HGFEDCBA
	vpaddd		%zmm4, %zmm2, %zmm2{%k6}

	kmovd		%k4, %r10d		// move hi/lo surrogate masks to general purpose registers
	kmovd		%k5, %r11d		// this moves the validation off p0/p5, making it no longer
						// part of the critical path

	// check for mismatched surrogates
	add		%r10d, %eax		// high surrogates shifted in place of low surrogates w/ carry
	add		%r10d, %eax		// use 2x add so p6 can be used instead of p1
	xor		%r11d, %eax		// do these match the low surrogates?
	jnz		.Lfail			// if yes, encoding error, else we have eax == 0

	shr		$30, %r10d		// set up !ZF == high surrogate carry out

	// transcode 1, 2, 3, and 4 byte characters at once!
	// at this point, k4:k5 indicates the dwords in zmm2:zmm1 that hold low surrogates
3:	setnz		%al			// set up surrogate carry out
	vpmultishiftqb	%zmm1, %zmm23, %zmm3	// HGFEDCBA PNMLKJHG VUTSRQPN 00000WVU
	vpmultishiftqb	%zmm2, %zmm23, %zmm4

	vpbroadcastd	.L00010101(%rip), %zmm21

	knotd		%k2, %k2		// which of zmm2:zmm1 are ASCII characters?
	kandnd		%k1, %k5, %k5		// which of zmm2:zmm1 shall be written out? (not lo surr, not lookahead)
	kshiftrd	$16, %k3, %k7		// which of zmm2 are 1 or 2 byte characters?
	kshiftrd	$16, %k2, %k6		// which of zmm2 are ASCII characters?
	kshiftrd	$16, %k5, %k4		// which of zmm2 shall be written out?

	vpslld		$16, %zmm28, %zmm16{%k3} // 2 byte: 80c00000  3 byte: 8080e000  4 byte: 808080f0
	vpslld		$16, %zmm28, %zmm17{%k7} // same for high characters
	vpblendmd	%zmm21, %zmm20, %zmm5{%k5} // prepare masks for desired byte detection:
	vpblendmd	%zmm21, %zmm20, %zmm6{%k4} // if written out 00010101 else ffffffff
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
	// we could use vpcompressb zmm, mem{k} here, but it's really slow for some reason.
	vpcmpnltub	%zmm5, %zmm3, %k2	// which bytes in zmm1 do we want to deposit?  (nonzero bytes,
	vpcmpnltub	%zmm6, %zmm4, %k3	// ..., the MSB of each dword but no lo surr. and no lookahead)
	kmovq		%k2, %r10		// get the masks so we can find out how long the output is
	kmovq		%k3, %r11
	vpcompressb	%zmm3, %zmm3{%k2}{z}	// smoosh characters together
	vpcompressb	%zmm4, %zmm4{%k3}{z}

	pext		%r10, %r10, %rbx	// mask of valid bytes in zmm3
	popcnt		%r10, %r10		// number of UTF-8 bytes in zmm3
	kmovq		%rbx, %k2

	pext		%r11, %r11, %rbx	// mask of valid bytes in zmm4
	popcnt		%r11, %r11		// number of UTF-8 bytes in zmm4
	kmovq		%rbx, %k3

	vmovdqu8	%zmm3, (%rdi){%k2}	// deposit into output
	vmovdqu8	%zmm4, (%rdi, %r10, 1){%k3}

	add		%r10, %rdi		// advance past first half of output
	add		%r11, %rdi		// advance past second half of output

	cmp		$32, %rdx		// enough left for another round?
	jae		.Loop			// if yes, go again!

	sub		%r9, %rdi		// advance past padding bytes (for tail treatment)
.Ltail:	test		%edx, %edx		// anything left to decode at all?
	jnz		4f			// if yes, process it

.Lend:	pop		%rax			// start of input buffer
	add		%r9, %rdi		// subtract padding characters from buffer end
	sub		%rax, %rsi		// input length = buffer end - buffer start
	sar		$1, %rsi		// in characters
	lea		(%rsi, %r9, 1), %rax	// ... - number of padding characters
	sub		%r8, %rdi		// output length = buffer end - buffer start
	mov		%rdi, (%rcx)		// deposit into *outlen

	pop		%rbx
	vzeroupper
	ret

	// Process <31 words tail.
	// Strategy: pad remaining input with U+0000 and
	// remember negated number of padding bytes in r9
4:	mov		$0x7fffffff, %r9d
	bzhi		%edx, %r9d, %r9d	// r9d = 0x7fffffff & (1 << edx) - 1
	kmovd		%r9d, %k1		// mask of words still in the buffer
	vmovdqu16	(%rsi), %zmm0{%k1}{z}	// manually load tail padded with U+000000
	lea		-31(%rdx), %r9		// set up r9 with the neg. number of padding characters
	xor		%edx, %edx		// no further input to be processed
	jmp		.Llastiteration

	// decoding failure: dock input to length of its valid prefix and run final iteration
	// this is very similar to the tail processing code
.Lfail:	xor		%r11d, %eax		// restore high surrogates+carry
	andn		%r11d, %eax, %eax	// low surrogates without high surrogates before?
	shr		%r11d			// low surrogates in place of high surrogates
	andn		%r10d, %r11d, %r11d	// high surrogates without low surrogates after?
	or		%r11d, %eax		// either case?

	tzcnt		%eax, %edx		// where did the decoding error occur?
	mov		$0x7fffffff, %eax
	bzhi		%edx, %eax, %eax	// eax = 0x7fffffff & (1 << edx) - 1

	kmovd		%eax, %k1		// mask of words still in the buffer
	kmovd		%k5, %eax		// fake the surrogate carry: if an error occured due to
	and		$1, %eax		// the carry not matching a leading low surrogate, the
						// final iteration would produce zero bytes anyway
	vmovdqu16	%zmm0, %zmm0{%k1}{z}	// mask input to valid prefix padded with U+000000
	lea		-31(%rdx), %r9		// do not count NUL byte padding in output length
	xor		%edx, %edx		// no further input to be processed
	jmp		.Lfailiteration

	.size		utf16le_to_utf8_avx512, .-utf16le_to_utf8_avx512


// return the length of the output buffer for transcoding the given number of characters
	.type		utf16le_to_utf8_buflen_avx512, @function
	.globl		utf16le_to_utf8_buflen_avx512
utf16le_to_utf8_buflen_avx512:
	lea		(%rdi,%rdi,2), %rax	// up to 3 bytes for each word
						// plus 95 byte we might store past the end
	ret

	.size		utf16le_to_utf8_buflen_avx512, .-utf16le_to_utf8_buflen_avx512
