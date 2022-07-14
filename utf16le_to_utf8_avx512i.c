#include <uchar.h>
#include <immintrin.h>
#include <stdint.h>

extern size_t
utf16le_to_utf8_avx512i(unsigned char outbuf[restrict], const char16_t inbuf[restrict], size_t inlen, size_t *outlen)
{
	__m512i in;
	__mmask32 inmask, carry = _cvtu32_mask32(0), trunk = _cvtu32_mask32(0x7fffffff);
	const char16_t *inbuf_orig = inbuf;
	unsigned char *outbuf_orig = outbuf;
	int adjust = 0;

	while (inlen >= 32) {
		__m512i hi, lo, fc00masked, taghi, taglo, mslo, mshi, outlo, outhi;
		__mmask64 ignlo, ignhi, is12blo, is12bhi, is1blo, is1bhi, wantlo, wanthi;
		__mmask32 is234byte, is12byte, is1byte, carryout, hisurr, losurr;
		int advlo, advhi;

		in = _mm512_loadu_epi16(inbuf);
		inlen -= 31;

	lastiteration:
		inbuf += 31;

	failiteration:
		is234byte = _mm512_mask_cmp_epu16_mask(trunk, in, _mm512_set1_epi16(0x0080), _MM_CMPINT_NLT);
		if (_ktestz_mask32_u8(is234byte, is234byte)) {
			/* fast path for ASCII only */
			__m256i out;

			out = _mm512_cvtepi16_epi8(in);
			_mm256_storeu_epi8(outbuf, out);
			outbuf += 31;
			carry = _cvtu32_mask32(0);
			continue;
		}

		is12byte = _mm512_cmp_epu16_mask(in, _mm512_set1_epi16(0x0800), _MM_CMPINT_LT);
		if (_ktestc_mask32_u8(is12byte, trunk)) {
			/* fast path for 1 and 2 byte only */
			__m512i twobytes, out;
			__mmask64 smoosh;

			twobytes = _mm512_ternarylogic_epi32(
			    _mm512_slli_epi16(in, 8),
			    _mm512_srli_epi16(in, 6),
			    _mm512_set1_epi16(0x3f3f),
			    0xa8 /* (A|B)&C */);
			in = _mm512_mask_add_epi16(in, is234byte, twobytes, _mm512_set1_epi16(0x80c0));
			smoosh = _mm512_cmp_epu8_mask(in, _mm512_set1_epi16(0x0800), _MM_CMPINT_NLT);
			out = _mm512_maskz_compress_epi8(smoosh, in);
			_mm512_storeu_epi8(outbuf, out);
			outbuf += 31 + __builtin_popcountl(_cvtmask32_u32(is234byte));
			carry = _cvtu32_mask32(0);
			continue;
		}

		lo = _mm512_cvtepu16_epi32(_mm512_castsi512_si256(in));
		hi = _mm512_cvtepu16_epi32(_mm512_extracti32x8_epi32(in, 1));
		carryout = _cvtu32_mask32(0);

		taglo = taghi = _mm512_set1_epi32(0x8080e000);

		fc00masked = _mm512_and_epi32(in, _mm512_set1_epi16(0xfc00));
		hisurr = _mm512_mask_cmp_epu16_mask(trunk, fc00masked, _mm512_set1_epi16(0xd800), _MM_CMPINT_EQ);
		losurr = _mm512_cmp_epu16_mask(fc00masked, _mm512_set1_epi16(0xdc00), _MM_CMPINT_EQ);

		/*
		 * in the asm implementation, these alias hisurr and losurr and are
		 * by construction zero when the following branch is not taken.
		 */
		ignlo = _cvtu64_mask64(0);
		ignhi = _cvtu64_mask64(0);

		if (!_kortestz_mask32_u8(hisurr, losurr)) {
			/* handle surrogates */
			__m512i his, los;
			__mmask32 hinolo, lonohi;
			__mmask16 hisurrhi, hisurrlo;

			los = _mm512_alignr_epi32(hi, lo, 1);
			his = _mm512_alignr_epi32(lo, hi, 1);

			hinolo = _kandn_mask32(_kshiftri_mask32(losurr, 1), hisurr);
			lonohi = _kandn_mask32(_kor_mask32(carry, _kadd_mask32(hisurr, hisurr)), losurr);
			if (!_kortestz_mask32_u8(hinolo, lonohi)) {
				/* handle encoding error */
				inlen = _tzcnt_u32(_cvtmask32_u32(_kor_mask32(hinolo, lonohi)));
				inmask = _cvtu32_mask32(0x7fffffff & (1 << inlen) - 1);
				in = _mm512_maskz_mov_epi16(inmask, in);
				adjust = (int)inlen - 31;
				inlen = 0;
				goto failiteration;
			}

			hisurrlo = (__mmask16)hisurr;
			hisurrhi = (__mmask16)_kshiftri_mask32(hisurr, 16);
			taglo = _mm512_mask_mov_epi32(taglo, hisurrlo, _mm512_set1_epi32(0x808080f0));
			taghi = _mm512_mask_mov_epi32(taghi, hisurrhi, _mm512_set1_epi32(0x808080f0));

			lo = _mm512_mask_slli_epi32(lo, hisurrlo, lo, 10);
			hi = _mm512_mask_slli_epi32(hi, hisurrhi, hi, 10);
			los = _mm512_add_epi32(los, _mm512_set1_epi32(0xfca02400));
			his = _mm512_add_epi32(his, _mm512_set1_epi32(0xfca02400));
			lo = _mm512_mask_add_epi32(lo, hisurrlo, lo, los);
			hi = _mm512_mask_add_epi32(hi, hisurrhi, hi, his);

			carryout = _kshiftri_mask32(hisurr, 30);
			ignlo = _mm512_movepi8_mask(_mm512_movm_epi32((__mmask16)losurr));
			ignhi = _mm512_movepi8_mask(_mm512_movm_epi32((__mmask16)_kshiftri_mask32(losurr, 16)));
		}

		mslo = _mm512_multishift_epi64_epi8(_mm512_set1_epi64(0x20262c3200060c12), lo);
		mshi = _mm512_multishift_epi64_epi8(_mm512_set1_epi64(0x20262c3200060c12), hi);
		carry = carryout;

		is1byte = _knot_mask32(is234byte);
		is1blo = (__mmask16)is1byte;
		is1bhi = (__mmask16)_kshiftri_mask32(is1byte, 16);
		is12blo = (__mmask16)is12byte;
		is12bhi = (__mmask16)_kshiftri_mask32(is12byte, 16);

		taglo = _mm512_mask_mov_epi32(taglo, is12blo, _mm512_set1_epi32(0x80c00000));
		taghi = _mm512_mask_mov_epi32(taghi, is12bhi, _mm512_set1_epi32(0x80c00000));
		mslo = _mm512_ternarylogic_epi32(mslo, _mm512_set1_epi32(0x3f3f3f3f), taglo, 0xea); /* A&B|C */
		mshi = _mm512_ternarylogic_epi32(mshi, _mm512_set1_epi32(0x3f3f3f3f), taghi, 0xea);
		mslo = _mm512_mask_slli_epi32(mslo, is1blo, lo, 24);
		mshi = _mm512_mask_slli_epi32(mshi, is1bhi, hi, 24);

		wantlo = _mm512_cmp_epu8_mask(mslo, _mm512_set1_epi32(0x00010101), _MM_CMPINT_NLT);
		wanthi = _mm512_cmp_epu8_mask(mshi, _mm512_set1_epi32(0x00010101), _MM_CMPINT_NLT);
		wantlo = _kandn_mask64(ignlo, wantlo);
		wanthi = _kandn_mask64(ignhi, wanthi);

		outlo = _mm512_maskz_compress_epi8(wantlo, mslo);
		outhi = _mm512_maskz_compress_epi8(wanthi, mshi);
		advlo = __builtin_popcountll(_cvtmask64_u64(wantlo));
		advhi = __builtin_popcountll(_cvtmask64_u64(wanthi) << 4);
		_mm512_storeu_epi8(outbuf, outlo);
		_mm512_storeu_epi8(outbuf + advlo, outhi);
		outbuf += advlo + advhi;
	}

	if (inlen != 0) {
		inmask = _cvtu32_mask32(0x7fffffff & (1 << inlen) - 1);
		in = _mm512_maskz_loadu_epi16(inmask, inbuf);
		adjust = (int)inlen - 31;
		inlen = 0;
		goto lastiteration;
	}

	*outlen = outbuf - outbuf_orig + adjust;
	return (inbuf - inbuf_orig + adjust);
}

extern size_t
utf16le_to_utf8_buflen_avx512i(size_t n)
{
	return (4*n + 95);
}
