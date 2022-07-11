	.section .rodata
	.balign 32
.Lfc00:	.int		0xfc00fc00
.Ld800:	.int		0xd800d800
.Ldc00:	.int		0xdc00dc00

	.text
	.globl utf16le_validate_avx512
	.type utf16le_validate_avx512, @function
utf16le_validate_avx512:
	// rdi: buf
	// rsi: len
	// invariant: buf+len remains constant at loop entry
	vpbroadcastd	.Lfc00(%rip), %zmm31
	vpbroadcastd	.Ld800(%rip), %zmm30
	vpbroadcastd	.Ldc00(%rip), %zmm29

	mov		%rsi, %r9		// remember buffer start for return value

	mov		$0x7fffffff, %edx	// mask ignoring the lookahead
	kmovd		%edx, %k1
	xor		%eax, %eax		// low surrogate carry

	cmp		$32, %rsi		// can we load a zmm register?
	jb		.Ltail

.Loop:	vpandd		(%rdi), %zmm31, %zmm0	// mask input to top 6 bits
	add		$62, %rdi
	sub		$31, %rsi
	vpcmpequw	%zmm30, %zmm0, %k2{%k1}	// 0xd800 <= input < 0xdc000 (high surrogate, except lookahead)
	vpcmpequw	%zmm29, %zmm0, %k3
	kmovd		%k2, %ecx
	kmovd		%k3, %edx
	lea		(%rax, %rcx, 2), %eax	// high surrogates + carry
	cmp		%edx, %eax		// mismatched surrogates?
	jnz		.Lfail

	shr		$31, %eax		// compute surrogate carry out

	cmp		$32, %rsi		// can we load another zmm register
	jae		.Loop

	// process the last few elements
.Ltail:	mov		$0x7fffffff, %ecx
	bzhi		%esi, %ecx, %ecx	// ecx = 0x7fffffff & (1 << ecx) - 1
	kmovd		%ecx, %k4		// mask of words we want to load
	vmovdqu16	(%rdi), %zmm0{%k4}	// load relevant tail bytes

	vpandd		%zmm31, %zmm0, %zmm0
	vpcmpequw	%zmm30, %zmm0, %k2{%k1}
	vpcmpequw	%Zmm29, %zmm0, %k3
	kmovd		%k2, %ecx
	kmovd		%k3, %edx
	lea		(%rax, %rcx, 2), %eax
	cmp		%edx, %eax
	jnz		.Lfail

	mov		%r9, %rax		// full input has been validated
	vzeroupper
	ret

.Lfail:	andn		%edx, %eax, %eax	// low surrogates without high surrogate before?
	shr		%edx			// low surrogates in place of high surrogates
	andn		%ecx, %edx, %edx	// high surrogates without low surrogate after?
	or		%edx, %eax		// either case?

	tzcnt		%eax, %eax		// where did the decoding error occur?
	lea		-31(%rsi, %rax, 1), %rax // calculate number of valid characters
	sub		%r9, %rax
	vzeroupper
	ret

	.size		utf16le_validate_avx512, .-utf16le_validate_avx512

