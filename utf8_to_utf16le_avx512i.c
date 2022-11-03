#include <uchar.h>
#include <immintrin.h>
#include <stdint.h>

extern size_t
utf8_to_utf16le_avx512i(char16_t outbuf[restrict],
    const unsigned char inbuf[restrict], size_t inlen, size_t *outlen)
{
	char16_t *restrict orig_outbuf = outbuf;
	const unsigned char *restrict orig_inbuf = inbuf;
	__mmask64 b = _cvtu64_mask64(0xffffffffffffffff);

	while (inlen > 0) {
		__m512i in, wstripped, P, Wend, Wminus1, Wminus2, Wsum, Wout;
		__m512i Wshifted, Wd800;
		__mmask64 m1, m1234, m234, m3, m34, m4, mc1c2, mc, mprocessed;
		__mmask64 mend, mout, mplus3;
		__mmask32 Mlo, Mhi, M3, M_U800, M3s, M4s, Mout;
		size_t nin, nout;

		if (inlen >= 64) {
			in = _mm512_loadu_epi8(inbuf);

			m1 = _mm512_cmp_epu8_mask(in, _mm512_set1_epi8(0x80), _MM_CMPINT_LT);
			if (_kortestc_mask32_u8(m1, m1)) { /* all ASCII? */
				__m512i out;

				out = _mm512_cvtepu8_epi16(_mm512_castsi512_si256(in));
				_mm512_storeu_epi8(outbuf, out);

				inbuf += 32;
				inlen -= 32;
				outbuf += 32;
				continue;
			}

			m234 = _mm512_cmp_epu8_mask(in, _mm512_set1_epi8(0xc0), _MM_CMPINT_NLT);
			m34 = _mm512_cmp_epu8_mask(in, _mm512_set1_epi8(0xe0), _MM_CMPINT_NLT);
			mc1c2 = _mm512_mask_cmp_epu8_mask(m234, in, _mm512_set1_epi8(0xc2), _MM_CMPINT_LT);
			if (!_ktestz_mask64_u8(mc1c2, mc1c2)) { /* C1/C2 present? */
			has_c1c2:
				inlen = _tzcnt_u64(mc1c2);
				goto tail;
			}

			m1234 = m1 | m234;
			if (_ktestz_mask64_u8(m34, m34)) { /* all 1/2 byte? */
				__m512i minusc2, first, second, out;
				size_t not_processed;

				mend = ~m234;
				minusc2 = _mm512_maskz_sub_epi8(m234, in, _mm512_set1_epi8(0xc2));
				first = _mm512_cvtepu8_epi16(_mm512_castsi512_si256(
				    _mm512_maskz_compress_epi8(m1234, minusc2)));
				second = _mm512_cvtepu8_epi16(_mm512_castsi512_si256(
				    _mm512_maskz_compress_epi8(mend, in)));
				out = _mm512_add_epi16(second, _mm512_slli_epi16(first, 6));
				_mm512_storeu_epi16(outbuf, out);

				/* check for sequencing errors */
				if (mend + mend + 1 != m1234) {
					uint64_t mismatch, lead_before;

					mismatch = mend + mend + 1 ^ m1234;
					inlen = _tzcnt_u64(mismatch);
					/* lead byte where follow byte expected? */
					if (m1234 & 1ULL << inlen) {
						lead_before = m1234 & (1ULL << inlen) - 1;
						inlen = 63 - _lzcnt_u64(lead_before);
					}

					goto tail;
				}

				mout = _cvtu64_mask64(_pdep_u64(0xffffffff, (uint64_t)mend));
				not_processed = _lzcnt_u64((uint64_t)mout);
				inbuf += 64 - not_processed;
				inlen -= 64 - not_processed;
				outbuf += 32;
				continue;
			}
		} else { /* handle tail bytes: no fast paths, expedited processing. */
		tail:	b = _cvtu64_mask64((1ULL << inlen) -1);
			in = _mm512_maskz_loadu_epi8(b, inbuf);
			m1 = _mm512_cmp_epu8_mask(in, _mm512_set1_epi8(0x80), _MM_CMPINT_LT);
			m234 = _mm512_cmp_epu8_mask(in, _mm512_set1_epi8(0xc0), _MM_CMPINT_NLT);
			m34 = _mm512_cmp_epu8_mask(in, _mm512_set1_epi8(0xe0), _MM_CMPINT_NLT);
			m1234 = m1 | m234;

			mc1c2 = _mm512_mask_cmp_epu8_mask(m234, in, _mm512_set1_epi8(0xc2), _MM_CMPINT_LT);
			if (!_ktestz_mask64_u8(mc1c2, mc1c2)) /* C1/C2 present? */
				goto has_c1c2;
		}

		m4 = _mm512_cmp_epu8_mask(in, _mm512_set1_epi8(0xf0), _MM_CMPINT_NLT);
		mplus3 = m4 << 3;
		mend = (mplus3 | m1234) >> 1 | mplus3;

		wstripped = _mm512_andnot_epi32(_mm512_maskz_mov_epi8(~m1, _mm512_set1_epi8(0xc0)), in);
		P = _mm512_cvtepu8_epi16(_mm512_castsi512_si256(_mm512_maskz_compress_epi8(mend, _mm512_set_epi8(
		    63, 62, 61, 60, 59, 58, 57, 56,
		    55, 54, 53, 52, 51, 50, 49, 48,
		    47, 46, 45, 44, 43, 42, 41, 40,
		    39, 38, 37, 36, 35, 34, 33, 32,
		    31, 30, 29, 28, 27, 26, 25, 24,
		    23, 22, 21, 20, 19, 18, 17, 16,
		    15, 14, 13, 12, 11, 10,  9,  8,
		     7,  6,  5,  4,  3,  2,  1,  0))));

		Wend =_mm512_maskz_permutexvar_epi8(_cvtu64_mask64(0x5555555555555555), P, wstripped);
		Wminus1 = _mm512_maskz_permutexvar_epi8(_cvtu64_mask64(0x5555555555555555),
		    _mm512_add_epi8(P, _mm512_set1_epi8(0xff)), _mm512_maskz_mov_epi8(~m1 >> 1, wstripped));
		Wminus2 = _mm512_maskz_permutexvar_epi8(_cvtu64_mask64(0x5555555555555555),
		    _mm512_add_epi8(P, _mm512_set1_epi8(0xfe)), _mm512_maskz_mov_epi8(
		    m34 & _cvtu64_mask64(0x3fffffffffffffff), wstripped));
		Wsum = _mm512_or_epi32(_mm512_slli_epi16(Wminus2, 12),
		    _mm512_or_epi32(_mm512_slli_epi16(Wminus1, 6), Wend));


		mc = (m234 << 1) | (m34 << 2) | mplus3;
		if (mc != ~m1234) { /* are continuation bytes where they should be? */
			uint64_t mismatch, lead_before;

			mismatch = mc ^ ~m1234;
			inlen = _tzcnt_u64(mismatch);

			/* lead byte where follow byte expected? */
			if (m1234 & 1ULL << inlen) {
				lead_before = m1234 & (1ULL << inlen) - 1;
				inlen = 63 - _lzcnt_u64(lead_before);
			}

			goto tail;
		}

		Mlo = _cvtu32_mask32(_pext_u64(mplus3, mend));
		Mhi = Mlo >> 1;

		Wshifted = _mm512_srli_epi16(Wsum, 4);
		Wout = _mm512_or_epi32(Wsum, _mm512_maskz_mov_epi16(Mlo, _mm512_set1_epi16(0xdc00)));
		Wout = _mm512_mask_add_epi16(Wout, Mhi, Wshifted, _mm512_set1_epi16(0xd7c0));

		/* check 3 and 4 byte sequences for correctness */
		m3 = m34 & ~m4;
		M3 = _cvtu32_mask32(_pext_u64(m3 << 2, mend));
		/* overlong 3 byte sequence? */
		M_U800 = M3 & _mm512_cmp_epu16_mask(Wout, _mm512_set1_epi16(0x0800), _MM_CMPINT_LT);
		Wd800 = _mm512_sub_epi16(Wout, _mm512_set1_epi16(0xd800));
		/* 3 byte sequence encodes surrogate? */
		M3s = M3 & _mm512_cmp_epu16_mask(Wd800, _mm512_set1_epi16(0x0800), _MM_CMPINT_LT);
		/* codepoint > U+10FFFF encoded? 5+ byte sequence encoded? */
		M4s = Mhi & _mm512_cmp_epu16_mask(Wd800, _mm512_set1_epi16(0x0400), _MM_CMPINT_NLT);

		if (M_U800 | M3s | M4s) {
			uint64_t where;

			where = M_U800 | M3s | M4s;
			inlen = _tzcnt_u64(_pdep_u64(where, m1234 | mplus3));

			goto tail;
		}

		Mout = ~(Mhi & _cvtu32_mask32(0x80000000));
		mprocessed = _cvtu64_mask64(_pdep_u64((uint64_t)Mout, b & mend));
		nin = 64 - _lzcnt_u64(mprocessed);
		nout = __builtin_popcountll(mprocessed);

		_mm512_mask_storeu_epi16(outbuf, _cvtu32_mask32((1ULL << nout) -1), Wout);
		inbuf += nin;
		inlen -= nin;
		outbuf += nout;
		continue;
	}

	*outlen = outbuf - orig_outbuf;
	return ((size_t)(inbuf - orig_inbuf));
}

size_t
utf8_to_utf16le_buflen_avx512i(size_t n)
{
	return (n);
}
