	.section .rodata
	.balign 4
.L0080:	.int		0x00800080
.L0800:	.int		0x08000800

.Lfc00:	.int		0xfc00fc00
.Ld800:	.int		0xd800d800
.Ldc00:	.int		0xdc00dc00

.L3f3f:	.int		0x3f3f3f3f
.L80c0:	.int		0x80c080c0

.L008080e0:
	.int		0x008080e0
.L808080f0:
	.int		0x808080f0
.L2840:	.int		0x00002840

	.balign 8
	// masks for vpmultishiftq
.Lms3:	.byte		12, 6, 0, 16, 44, 38, 32, 48
.Lms4:	.byte		24, 18, 12, 6, 56, 50, 44, 38

	.text
	.globl utf16le_to_utf8_avx512
	.type utf16le_to_utf8, @function
utf16le_to_utf8_avx512:
	// rdi: outbuf
	// rsi: inbuf
	// rdx: len
	// rcx: &outlen

	vpbroadcastd	.L0080(%rip), %zmm31
	vpbroadcastd	.L0800(%rip), %zmm30

//	vpbroadcastd	.Lfdd0(%rip), %zmm26
//	vpbroadcastd	.Lfdf0(%rip), %zmm25
//	vpbroadcastd	.Lfffe(%rip), %zmm24

	vpbroadcastd	.Lfc00(%rip), %zmm29
	vpbroadcastd	.Ld800(%rip), %zmm28
	vpbroadcastd	.Ldc00(%rip), %zmm27

	vpbroadcastd	.L3f3f(%rip), %zmm23
	vpbroadcastd	.L80c0(%rip), %zmm22

	vpbroadcastq	.Lms3(%rip), %zmm24	// 3 byte multi shift mask
	vpbroadcastd	.L008080e0(%rip), %zmm21

	vpbroadcastd	.L2840(%rip), %zmm26
	vpbroadcastd	.L808080f0(%rip), %zmm25

	mov		$0x7fffffff, %eax	// mask ignoring the lookahead
	kmovd		%eax, %k7

	mov		$0x1111111111111111, %rax // the least byte of each dword
	kmovq		%rax, %k6

	kxord		%k0, %k0, %k0		// low surrogate carry

	cmp 		$32, %rdx		// can we load a zmm register?
	jb		.Ltail

0:	vmovdqu16	(%rsi), %zmm0
	add		$62, %rsi		// advance to next block (sans lookahead)
	sub		$31, %rdx

	vpcmpnltuw	%zmm31, %zmm0, %k1	// zmm0 >= 0x0080?  (not ASCII character?)
	ktestd		%k1, %k1		// all ASCII characters?
	jnz		1f			// if not, do slower processing

	// fast path 1: all ASCII characters
	vpmovwb		%zmm0, %ymm0		// compress into ASCII
	vmovdqu8	%ymm0, (%rdi)		// and store into destination
	add		$31, %rdi
	kxord		%k0, %k0, %k0		// no high surrogates to carry over
	jmp		.Lnext			// and go to next iteration

1:	vpcmpltuw	%zmm30, %zmm0, %k2	// zmm0 >= 0x0800? (>2 byte character?)
	kandnd		%k1, %k2, %k3		// which of zmm0 are 2 byte characters?

	// process 1 and 2 byte encodings
	// input: 0000 0ABC DEFG HJKL
	vpsrlw		$6, %zmm0, %zmm4	// 00000000 000ABCDE
	vpsllw		$8, %zmm0, %zmm2	// DEFGHJKL 00000000
	vpternlogd	$0xea, %zmm23, %zmm4, %zmm2 // 00FGHJKL 000ABCDE
	vpaddw		%zmm22, %zmm2, %zmm0{%k3} // 10FGHJKL 110ABCDE or 00000000 0EFGHJKL

	ktestd		%k2, %k2		// all 1 or 2 byte characters?
	jnz		1f			// if not, do slower processing

	// fast path 2: 1 or 2 byte encodings only
	vpcmpnltub	%zmm30, %zmm0, %k2	// for each word: 01 if ASCII, 11 if 2byte
	vpcompressb	%zmm0, %zmm0{%k2}{z}	// smoosh all the characters together
	vmovdqu8	%zmm0, (%rdi)		// and store to destination buffer
	kmovd		%k1, %eax		// determine advance based on k1
	shl		$1, %eax		// ignore high bit (lookahead)
	popcnt		%eax, %eax		// count number of two byte characters
	lea		31(%rdi, %rax, 1), %rdi	// advance destination buffer

	kxord		%k0, %k0, %k0		// no high surrogates to carry over
	jmp		.Lnext

	// surrogates present?
1:	vpandd		%zmm29, %zmm0, %zmm3	// zmm0 & 0xfc00 (for surrogates)
	vpcmpequw	%zmm28, %zmm3, %k5{%k7}	// 0xd800 <= c0 < 0xdc00 (high surrogate, except lookahead)
	vpcmpequw	%zmm27, %zmm3, %k6	// 0xdc00 <= c0 < 0xe000 (low surrogate)

	vpaddw		%zmm26, %zmm0, %zmm0{%k5} // fix high surrogates to 00000xab cdEFGHJK
	vextracti64x4	$1, %zmm0, %ymm2	// obtain high 15 characters
	vpmovzxwd	%zmm0, %zmm1		// these are a mix of already translated 1/2 byte chars
	vpmovzxwd	%zmm2, %zmm2		// and untranslated 3 byte chars.  Will be fixed up later

	krshiftd	$16, %k2, %k4		// which of zmm2 is a 3 or 4 byte character?
	kortestd	%k5, %k6		// if surrogates are present
	jnz		1f			// go and handle them (slowest path)

	// fast path 3: 1, 2, or 3 byte encodings, no surrogates
	// input: ABCDEFGH JKLMNOPQ
	vpmultishiftqb	%zmm1, %zmm24, %zmm5	// 00000000 JKLMNOPQ CDEFGHJK 0000ABCD
	vpmultishiftqb	%zmm2, %zmm24, %zmm6
	vpandd		%zmm23, %zmm5, %zmm5	// 00000000 00LMNOPQ 00EFGHJK 0000ABCD
	vpandd		%zmm23, %zmm6, %zmm6
	vpord		%zmm21, %zmm5, %zmm1{%k2} // 00000000 10LMNOPQ 10EFGHJK 1110ABCD
	vpord		%zmm21, %zmm6, %zmm2{%k4} // ... and join with 1/2 byte cases

	// find encoding lengths and compress
	vptestmb	%zmm1, %zmm1, %k1	// which bytes are nonzero in k2
	vptestmb	%zmm2, %zmm2, %k2
	korq		%k6, %k1, %k1		// which bytes make up the encoding (1--3)?
	korq		%k6, %k2, %k2		// this takes care of NUL bytes in the input
	kmovq		%k1, %rax
	kmovq		%k2, %r8
	shl		$4, $r8			// ignore lookahead
	popcnt		%rax, %rax		// length of data in zmm1
	popcnt		%r8, %r8		// length of data in zmm2
	vpcompressb	%zmm1, (%rdi){%k1}{z}	// store the resulting UTF-8 test
	add		%rax, %rdi		// advance output to second half
	vpcompressb	%zmm2, (%rdi){%k2}{z}
	add		%r8, %rdi		// advance output past second half
	jmp		.Lnext

	// slow path: 1--4 byte encoding, surrogates
1:	vpbroadcastq	.Lms4(%rip), %zmm6	// 4 byte multi shift mask
	vpalignd	$1, %zmm2, %zmm1, %zmm3	// low surrogates shifted in place of high ones
	vpalignd	$1, %zmm1, %zmm2, %zmm4

	// check for mismatched surrogates
	kaddd		%k5, %k5, %k1		// high surrogate shifted in place of low surrogate
	kord		%k0, %k1, %k1		// with carry applied
	kshiftrd	$1, %k6, %k0		// low surrogate shifted in place of high surrogate
	kandnd		%k5, %k0, %k0		// high surrogate without low surrogate after?
	kandnd		%k6, %k1, %k1		// low surrogate without high surrogate before?
	kortestd	%k0, %k1		// either case (mismatched or lone surrogate?)
	jnz		.Lfail

	// process 4 byte sequences
	// input:  (hi) 110110AB CDEFGHJK  (lo) 110111LM NOPQRSTU
	// output: 10PQRSTU 10JKLMNO 10cdEFGH 11110xab
	// where xabcd = 0ABCD + 00001
	krshiftd	$16, %k5, %k1		// where are 4 byte chars in zmm2/zmm4
	vpslld		$16, %zmm1, %zmm1{%k5}	// if 4 byte, 00000xab cdEFGHJK 00000000 00000000
	vpslld		$16, %zmm2, %zmm2{%k1}
	vpslld		$6, %zmm3, %zmm3	// 00000000 00000000 LMNOPQRS TU000000
	vpslld		$6, %zmm4, %zmm4
	vpaddd		%zmm3, %zmm1, %zmm1{%k5} // 3 byte: 00000000 00000000 ABCDEFGH JKLMNOPQ
	vpaddd		%zmm4, %zmm2, %zmm2{%k1} // 4 byte: 00000xab cdEFGHJK LMNOPQRS TU000000

	vpblendmd	%zmm24, %zmm6, %zmm5{%k5} // 3 byte or 4 byte multishift mask
	vpblendmd	%zmm24, %zmm6, %zmm6{%k1}
	vpmultishiftq	%zmm1, %zmm5, %zmm5	// 3 byte: 00000000 JKLMNOPQ CDEFGHJK 0000ABCD
	vpmultishiftq	%zmm2, %zmm6, %zmm6	// 4 byte: NOPQRSTU GHJKLMNO abcdEFGH 00000xab
	vpblendmd	%zmm21, %zmm25, %zmm3{%k5} // 3 byte: 008080e0  4 byte: 808080f0
	vpblendmd	%zmm21, %zmm25, %zmm4{%k1}
	vpternlogd	$0xea, %zmm3, %zmm23, %zmm5 // 3 byte: 00000000 10LMNOPQ 10EFGHJK 1110ABCD
	vpternlogd	$0xea, %zmm4, %zmm23, %zmm6 // 4 byte: 10PQRSTU 10JKLMNO 10cdEFGH 11110xab
	vpblendmd	%zmm1, %zmm5, %zmm1{%k2}    // merge into already translated 1 and 2 byte characters
	vpblendmd	%zmm2, %zmm6, %zmm2{%k4}
