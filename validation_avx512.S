	.section .rodata
	.balign 32
.Lperm:	.byte		1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16
	.byte		17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 0
.Lfc00:	.int		0xfc00fc00
.Lfdd0:	.int		0xfdd0fdd0
.Lfffe:	.int		0xfffefffe
.Lfdf0:	.int		0xfdf0fdf0
.Ld800:	.int		0xd800d800
.Ldc00:	.int		0xdc00dc00

	.text
	.globl utf16le_validate_avx512
	.type utf16le_validate_avx512, @function
utf16le_validate_avx512:
	// rdi: buf
	// rsi: len
	// invariant: buf+len remains constant at loop entry
	mov		%rsi, %rax		// remember len for return value

	vpbroadcastd	.Ld800(%rip), %zmm31
	vpbroadcastd	.Lfc00(%rip), %zmm25
	vpbroadcastd	.Lfdd0(%rip), %zmm26
	vpbroadcastd	.Lfffe(%rip), %zmm28
	vpbroadcastd	.Lfdf0(%rip), %zmm27

	mov		$0x7fffffff, %edx	// mask ignoring the lookahead
	kmovd		%edx, %k5
	kxord		%k6, %k6, %k6		// low surrogate carry

	vpbroadcastd	.Ldc00(%rip), %zmm30
	vpmovzxbw	.Lperm(%rip), %zmm29

	mov		$31, %ecx		// block length
	cmp		$32, %rsi		// can we load a zmm register?
	jb		.Ltail

0:	vmovdqu16	(%rdi), %zmm0
	lea		(%rdi, %rcx, 2), %rdi	// advance to next block (sans lookahead)
	sub		%rcx, %rsi

	// detect various special code points
1:	vpcmpleuw	%zmm0, %zmm31, %k2	// 0xd800 <= c0 (any interesting characters present?)
	ktestd		%k2, %k2		// if not, there's nothing to validate
	jz		.Ltrivial

	vpandd		%zmm25, %zmm0, %zmm2	// zmm0 & 0xfc00 (for surrogates)
	vpcmpleuw	%zmm0, %zmm26, %k4	// 0xfdd0 <= c0 (noncharacter)
	vpcmpnltuw	%zmm28, %zmm0, %k1	// c0 <= 0xfffe (noncharacter)
	vpcmpequw	%zmm31, %zmm2, %k2{%k5}	// 0xd800 <= c0 < 0xdc00 (high surrogate, except lookahead)
	vpcmpequw	%zmm30, %zmm2, %k3	// 0xdc00 <= c0 < 0xe000 (low surrogate)
	vpcmpltuw	%zmm27, %zmm0, %k4{%k4}	// 0xfdd0 <= c0 < 0xfdf0 (noncharacter)

	kortestd	%k2, %k3		// are surrogates involved?
	jz		.Lsimple		// if not, skip surrogate processing

	// detect noncharacters in high planes (i.e. U+XFFFE and U+XFFFF)
	vpermw		%zmm0, %zmm29, %zmm2	// zmm0 shifted right by one word
	vpsllw		$10, %zmm0, %zmm1	// high surrogates shifted into position
	vpsubw		%zmm30, %zmm1, %zmm1	// high surrogate - 0xdc00 (low surrogate prefix)
	vpaddw		%zmm1, %zmm2, %zmm1	// high surrogate << 16 + low surrogates

	// detect mismatched surrogates
	kaddd		%k2, %k2, %k0
	kord		%k6, %k0, %k0		// apply carry from previous iteration
	kshiftrd	$1, %k3, %k6
	kandnd		%k2, %k6, %k6		// high surrogate without low surrogate after?
	kandnd		%k3, %k0, %k0		// low surrogate without high surrogate before?
	kord		%k0, %k6, %k6		// either case? (mismatched surrogate)

	vpcmpnltuw	%zmm28, %zmm1, %k3{%k2}	// noncharacter in high plane?
	kord		%k3, %k4, %k4		// BMP or high noncharacter present?
	kord		%k1, %k6, %k1		// mismatched surrogates or BMP noncharacters?

.Lsimple:
	kortestd	%k1, %k4		// any encoding error present?
	jnz		.Lfail

.Ltrivial:
	kshiftrd	$30, %k2, %k6		// high surrogate carry
	cmp		$32, %rsi		// can we load a zmm register?
	jae		0b			// if yes, validate next 31 characters

.Ltail:	test		%rsi, %rsi		// all done?
	je		.Lend			// if yes, mark entire buffer as valid

	bzhi		%esi, %edx, %edx	// edx = (1 << esi) - 1
	kmovd		%edx, %k1

	vmovdqu16	(%rdi), %zmm0{%k1}{z}	// load tail into buffer (padded with U+0000)
	mov		%esi, %ecx		// remember tail block length
	xor		%esi, %esi		// no input remains (preserve invariant)
	jmp		1b			// process tail

.Lfail:	kord		%k4, %k1, %k1		// join the two error masks
	kmovd		%k1, %edx
	tzcnt		%edx, %edx		// when did the noncharacter occur?
	addl		%ecx, %esi		// compute valid prefix length
	addq		%rdx, %rax
	subq		%rsi, %rax

.Lend:	vzeroupper
	ret

	.size		utf16le_validate_avx512, .-utf16le_validate_avx512

