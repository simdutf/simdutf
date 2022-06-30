	.section .rodata
	.balign 32
.Lperm:	.byte		1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16
	.byte		17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 0
.L0080:	.int		0x00800080
.L0800:	.int		0x08000800

.Lfdd0:	.int		0xfdd0fdd0
.Lfdf0:	.int		0xfdf0fdf0
.Lfffe:	.int		0xfffefffe

.Lfc00:	.int		0xfc00fc00
.Ld800:	.int		0xd800d800
.Ldc00:	.int		0xdc00dc00

.L3f1f:	.int		0x3f1f3f1f
.L80c0:	.int		0x80c080c0

.L003f3f0f:
	.int		0x003f3f0f
.L008080e0:
	.int		0x008080e0

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

	vpbroadcastd	.Lfdd0(%rip), %zmm26
	vpbroadcastd	.Lfdf0(%rip), %zmm25
	vpbroadcastd	.Lfffe(%rip), %zmm24

	vpbroadcastd	.Lfc00(%rip), %zmm29
	vpbroadcastd	.Ld800(%rip), %zmm28
	vpbroadcastd	.Ldc00(%rip), %zmm27

	vpbroadcastd	.L3f1f(%rip), %zmm23
	vpbroadcastd	.L80c0(%rip), %zmm22

	vpbroadcastd	.L000f3f3f(%rip), %zmm21
	vpbroadcastd	.L008080e0(%rip), %zmm20

	mov		$0x7fffffff, %eax	// mask ignoring the lookahead
	kmovd		%eax, %k7

	mov		$0x1111111111111111, %rax // the least byte of each dword
	kmovq		%rax, %k6

	cmp 		$32, %rdx		// can we load a zmm register?
	jb		.Ltail

0:	vmovdqu16	(%rsi), %zmm0
	add		$62, %rsi		// advance to next block (sans lookahead)
	sub		$31, %rdx

	// fast path 1: all ASCII characters?
	vpcmpltuw	%zmm31, %zmm0, %k1	// zmm0 < 0x0080?  (ASCII character?)
	kortestd	%k1, %k1
	jc		.L1byte

	// fast path 2: all one or two byte?
	vpcmpltuw	%zmm30, %zmm0, %k2	// zmm0 < 0x0800? (two byte character?)
	kortestd	%k2, %k2
	jc		.L2byte

	// noncharacters present?
	vpcmpngtuw	%zmm26, %zmm0, %k5	// 0xfdd0 <= c0
	vpcmpltuw	%zmm25, %zmm0, %k5{%k5}	// 0xfdd0 <= c0 < 0xfdf0 (noncharacter)
	vpcmpnltuw	%zmm24, %zmm0, %k0	// 0xfffe <= c0 (noncharacter)

	kortestd	%k5, %k0
	jnz		.Lfail

	// fast path 3: surrogates present?
	vpandd		%zmm29, %zmm0, %zmm1	// zmm0 & 0xfc00 (for surrogates)
	vpcmpequw	%zmm28, %zmm0, %k3{%k7}	// 0xd800 <= c0 < 0xdc00 (high surrogate, except lookahead)
	vpcmpequw	%zmm27, %zmm0, %k4	// 0xdc00 <= c0 < 0xe000 (low surrogate)

	kortestd	%k3, %k4		// if no surrogates, are present
	jz		.L3byte			// all encodings fit in 3 bytes

	// slow path: handle surrogates
	ud2 // ...

	// fast path 1: all ASCII characters
.L1byte:vpmovwb		%zmm0, %ymm0		// compress into ASCII
	vmovdqu8	%ymm0, (%rdi)		// and store into destination
	add		$31, %rdi
	jmp		.Lnext			// and go to next iteration

	// fast path 2: 1 or 2 byte encodings
	// input for 2 byte encoding: 0000 0ABC DEFG HJKL
.L2byte:knotd		%k1, %k1		// 2byte character?
	vpsrlw		$6, %zmm0, %zmm1	// 0000 0000 000A BCDE
	vpsllw		$8, %zmm0, %zmm2	// DEFG HJKL 0000 0000
	vpord		%zmm2, %zmm1, %zmm1	// DEFG HJKL 000A BCDE
	vpandd		%zmm23, %zmm1, %zmm1	// 00FG HJKL 000A BCDE
	vpaddw		%zmm22, %zmm1, %zmm0{%k1} // 10FG HJKL 110A BCDE or 0000 0000 0EFG HJKL
	vpcmpnltub	%zmm30, %zmm0, %k2	// for each word: 01 if ASCII, 11 if 2byte
	vpcompressb	%zmm0, %zmm0{%k2}{z}	// smoosh all the characters together
	vmovdqu8	%zmm0, (%rdi)		// and store to destination buffer
	kmovd		%k1, %eax		// determine advance based on k1
	shl		$1, %eax		// ignore high bit (lookahead)
	popcnt		%eax, %eax		// count number of two byte characters
	lea		31(%rdi, %rax, 1), %rdi	// advance destination buffer
	jmp		.Lnext

	// fast path 3: 1, 2, or 3 byte encodings, no surrogates
.L3byte:vextracti64x4	$1, %zmm0, %ymm2	// obtain high 15 characters
	vpmovzxwd	%ymm0, %zmm1		// zero extend into dwords for further processing
	vpmovzxwd	%ymm2, %zmm2

	// process 1 and 2 byte encodings, same code as above
	kandnd		%k2, %k1, %k1		// k1: is two byte character
	vpsrlw		$6, %zmm0, %zmm3
	vpsllw		$8, %zmm0, %zmm2
	vpord		%zmm3, %zmm2, %zmm2
	vpandd		%zmm23, %zmm2, %zmm2
	vpaddw		%zmm22, %zmm2, %zmm0{%k1} // join 1 and 2 byte cases back into zmm0

	// process 3 byte encodings
	// input:	00000000 00000000 ABCDEFGH JKLMNOPQ
	// output:	00000000 10LMNOPQ 10EFGHJK 1110ABCD
	kaddq		%k6, %k6, %k5		// second least byte in each dword
	knotq		%k5, %k5		// XXXXXXXX XXXXXXXX -------- XXXXXXXX
	vpslld		$16, %zmm1, %zmm3	// ABCDEFGH JKLMNOPQ 00000000 00000000
	vpslld		$16, %zmm2, %zmm4
	vpslrd		$12, %zmm1, %zmm5	// 00000000 00000000 00000000 0000ABCD
	vpslrd		$12, %zmm2, %zmm6
	vpslld		$2, %zmm1, %zmm1	// 00000000 000000AB CDEFGHJK LMNOPQ00
	vpslld		$2, %zmm2, %zmm2
	vpaddb		%zmm3, %zmm5, %zmm1{%k5}// ABCDEFGH JKLMNOPQ CDEFGHJK 0000ABCD
	vpaddb		%zmm4, %zmm6, %zmm2{%k5}
	vpandd		%zmm21, %zmm1, %zmm1	// 00000000 00LMNOPQ 00EFGHJK 0000ABCD
	vpandd		%zmm21, %zmm2, %zmm2
	vpord		%zmm20, %zmm1, %zmm1	// 00000000 10LMNOPQ 10EFGHJK 1110ABCD
	vpord		%zmm20, %zmm2, %zmm2

	// join 1, 2, and 3 byte encodings
	vextracti64x4	$1, %zmm0, %ymm3
	krshiftd	$16, %k2, %k1		// is it a 1/2 byte encoding in ymm3?
	vpmovzxwd	%ymm0, %zmm1{%k2}	// merge 1/2 byte characters into 3 byte dwords
	vpmovzxwd	%ymm3, %zmm2{%k1}

	// find encoding lengths and compress
	vptestmb	%zmm2, %zmm2, %k2	// which bytes are nonzero in k2?
	vptestmb	%zmm1, %zmm1, %k1
	korq		%k6, %k2, %k2		// which bytes make up the encoding (1--3)?
	korq		%k6, %k1, %k1
	vpcompressb	%zmm2, %zmm2{%k2}{z}	// smoosh all the characters together
	vpcompressb	%zmm1, %zmm1{%k1}{z}
	kmovq		%k2, %r8
	kmovq		%k1, %rax
	shl		$4, %r8			// ignore lookahead
	popcnt		%rax, %rax		// length of data in zmm1
	popcnt		%r8, %r8		// length of data in zmm2
	vmovdqu8	%zmm1, (%rdi)		// store first half to output
	vmovdqu8	%zmm2, (%rdi, %rax, 1)	// store second half after first
	add		%rax, %rdi		// advance output pointer by first half
	add		%r8, %rdi		// advance output pointer by second half
	jmp		next
