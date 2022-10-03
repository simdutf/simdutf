#include "simdutf/icelake/intrinsics.h"

#include "scalar/utf16_to_utf8/valid_utf16_to_utf8.h"
#include "scalar/utf16_to_utf8/utf16_to_utf8.h"
#include "scalar/utf8_to_utf16/valid_utf8_to_utf16.h"
#include "scalar/utf8_to_utf16/utf8_to_utf16.h"
#include "scalar/utf8.h"
#include "scalar/utf16.h"

#include "simdutf/icelake/begin.h"
namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {
#ifndef SIMDUTF_ICELAKE_H
#error "icelake.h must be included"
#endif
#include "icelake/icelake-utf8-common.inl.cpp"
#include "icelake/icelake-macros.inl.cpp"
#include "icelake/icelake-from-valid-utf8.inl.cpp"
#include "icelake/icelake-utf8-validation.inl.cpp"
#include "icelake/icelake-from-utf8.inl.cpp"
#include "icelake/icelake-convert-utf16-to-utf32.inl.cpp"
#include "icelake/icelake-ascii-validation.inl.cpp"
#include "icelake/icelake-utf32-validation.inl.cpp"

} // namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf

namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {

simdutf_warn_unused bool implementation::validate_utf8(const char *buf, size_t len) const noexcept {
    avx512_utf8_checker checker{};
    const char* ptr = buf;
    const char* end = ptr + len;
    for (; ptr + 64 <= end; ptr += 64) {
        const __m512i utf8 = _mm512_loadu_si512((const __m512i*)ptr);
        checker.check_next_input(utf8);
    }
    {
       const __m512i utf8 = _mm512_maskz_loadu_epi8((1ULL<<(end - ptr))-1, (const __m512i*)ptr);
       checker.check_next_input(utf8);
    }
    checker.check_eof();
    return ! checker.errors();
}


simdutf_warn_unused bool implementation::validate_ascii(const char *buf, size_t len) const noexcept {
  const char* tail = icelake::validate_ascii(buf, len);
  if (tail) {
    return scalar::ascii::validate(tail, len - (tail - buf));
  } else {
    return false;
  }
}

simdutf_warn_unused bool implementation::validate_utf16(const char16_t *buf, size_t len) const noexcept {
    return scalar::utf16::validate(buf, len);
}

simdutf_warn_unused bool implementation::validate_utf32(const char32_t *buf, size_t len) const noexcept {
  const char32_t * tail = icelake::validate_utf32(buf, len);
  if (tail) {
    return scalar::utf32::validate(tail, len - (tail - buf));
  } else {
    return false;
  }
}


enum { SIMDUTF_FULL, SIMDUTF_TAIL, SIMDUTF_OK = -1 };

// Return -1 if ok, otherwise might return the location
// of the error.
template <int tail = SIMDUTF_FULL> static inline int process_block_utf8_to_utf16(const char *&in, char16_t *&out, size_t gap) {
  // constants
  __m512i mask_identity = _mm512_set_epi8(63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
  __m512i mask_c0c0c0c0 = _mm512_set1_epi32(0xc0c0c0c0);
  __m512i mask_80808080 = _mm512_set1_epi32(0x80808080);
  __m512i mask_f0f0f0f0 = _mm512_set1_epi32(0xf0f0f0f0);
  __m512i mask_e0e0e0e0 = _mm512_set1_epi32(0xe0e0e0e0);
  __m512i mask_c2c2c2c2 = _mm512_set1_epi32(0xc2c2c2c2);
  __m512i mask_ffffffff = _mm512_set1_epi32(0xffffffff);
  __m512i mask_d7c0d7c0 = _mm512_set1_epi32(0xd7c0d7c0);
  __m512i mask_dc00dc00 = _mm512_set1_epi32(0xdc00dc00);

  __mmask64 b = (tail == SIMDUTF_FULL) ? 0xFFFFFFFFFFFFFFFF : (uint64_t(1) << gap) - 1;
  __m512i input = (tail == SIMDUTF_FULL) ? _mm512_loadu_epi8(in) : _mm512_maskz_loadu_epi8(b, in);

  __mmask64 m1 = (tail == SIMDUTF_FULL) ? _mm512_cmplt_epu8_mask(input, mask_80808080) : _mm512_mask_cmplt_epu8_mask(b, input, mask_80808080);
  unsigned char pure_ascii;
  (void)_kortest_mask64_u8(~b, m1, &pure_ascii);
  if (pure_ascii) { // all ASCII
    if (tail == SIMDUTF_FULL) {
      // we convert a full 64-byte block, writing 128 bytes.
      __m512i input1 = _mm512_cvtepu8_epi16(_mm512_castsi512_si256(input));
      _mm512_storeu_si512(out, input1);
      out += 32;
      __m512i input2 = _mm512_cvtepu8_epi16(_mm512_extracti64x4_epi64(input, 1));
      _mm512_storeu_si512(out, input2);
      out += 32;
      in += 64;          // consumed 64 bytes
      return SIMDUTF_OK; // we are done
    } else {
      if (gap <= 32) {
        __m512i input1 = _mm512_cvtepu8_epi16(_mm512_castsi512_si256(input));
        _mm512_mask_storeu_epi16(out, ((uint64_t(1) << (gap)) - 1), input1);
        out += gap;
        in += gap;
      } else {
        __m512i input1 = _mm512_cvtepu8_epi16(_mm512_castsi512_si256(input));
        _mm512_storeu_si512(out, input1);
        out += 32;
        __m512i input2 = _mm512_cvtepu8_epi16(_mm512_extracti64x4_epi64(input, 1));
        _mm512_mask_storeu_epi16(out, ((uint32_t(1) << (gap - 32)) - 1), input2);
        out += gap - 32;
        in += gap;
      }
      return SIMDUTF_OK; // we are done
    }
  }
  // classify characters further
  __mmask64 m234 = _mm512_cmp_epu8_mask(mask_c0c0c0c0, input,
                                        _MM_CMPINT_LE); // 0xc0 <= input, 2, 3, or 4 leading byte
  __mmask64 m34 = _mm512_cmp_epu8_mask(mask_e0e0e0e0, input,
                                       _MM_CMPINT_LE); // 0xe0 <= input,  3 or 4 leading byte

  __mmask64 milltwobytes = _mm512_mask_cmp_epu8_mask(m234, input, mask_c2c2c2c2,
                                                     _MM_CMPINT_LT); // 0xc0 <= input < 0xc2 (illegal two byte sequence)
                                                                     // Overlong 2-byte sequence
  if (_ktestz_mask64_u8(milltwobytes, milltwobytes) == 0) {
    // Overlong 2-byte sequence
    return __tzcnt_u64(milltwobytes);
    // encoding error
  }
  if (_ktestz_mask64_u8(m34, m34) == 0) {
    __mmask64 m4 = _mm512_cmp_epu8_mask(input, mask_f0f0f0f0,
                                        _MM_CMPINT_NLT); // 0xf0 <= zmm0 (4 byte start bytes)
    __mmask64 mask_not_ascii = (tail == SIMDUTF_FULL) ? _knot_mask64(m1) : _kand_mask64(_knot_mask64(m1), b);

    __mmask64 mp1 = m234 << 1;
    __mmask64 mp2 = m34 << 2;
    __mmask64 mp3 = m4 << 3;
    __mmask64 mc = mp1 | mp2 | mp3; // expected continuation bytes
    __mmask64 m1234 = m1 | m234;
    // mismatched continuation bytes:
    if (mc != (b ^ m1234)) {
      // mismatched continuation bytes
      // continuation bytes at b ^ m1234, they should be at mc,
      // so if (b ^ m1234) &~ mc is non zero...
      // there is a continuation byte present where there should not be one
      int err1 = __tzcnt_u64(mc ^ (b ^ m1234));
      if (((b ^ m1234) & ~mc) != 0) {
        return err1;
      }
      // err1 will point at a missing continuation byte,
      // and the leading byte should be prior to it.
      uint64_t mpre = (uint64_t(1) << err1) - 1;
      //  lead byte that is missing a continuation byte
      uint64_t missing = (mpre & m234);
      return 64 - _lzcnt_u64(missing) - 1;
    }

    __mmask64 m4s3 = m4 << 3;

    // mend: identifying the last bytes of each sequence to be decoded
    __mmask64 mend = _kor_mask64((_kor_mask64(m4s3, _kor_mask64(m1, m234)) >> 1), m4s3);
    if (tail != SIMDUTF_FULL) {
      mend = _kor_mask64(mend, (uint64_t(1) << (gap - 1)));
    }
    __m512i last_and_third = _mm512_maskz_compress_epi8(mend, mask_identity);
    __m512i last_and_thirdu16 = _mm512_cvtepu8_epi16(_mm512_castsi512_si256(last_and_third));

    __m512i nonasciitags = _mm512_maskz_mov_epi8(mask_not_ascii, mask_c0c0c0c0); // ASCII: 00000000  other: 11000000
    __m512i clearedbytes = _mm512_andnot_si512(nonasciitags, input);             // high two bits cleared where not ASCII
    __m512i lastbytes = _mm512_maskz_permutexvar_epi8(0x5555555555555555, last_and_thirdu16,
                                                      clearedbytes); // the last byte of each character

    __mmask64 mask_before_non_ascii = _kshiftri_mask64(mask_not_ascii, 1);               // bytes that precede non-ASCII bytes
    __m512i indexofsecondlastbytes = _mm512_add_epi16(mask_ffffffff, last_and_thirdu16); // indices of the second last bytes
    __m512i beforeasciibytes = _mm512_maskz_mov_epi8(mask_before_non_ascii, clearedbytes);
    __m512i secondlastbytes = _mm512_maskz_permutexvar_epi8(0x5555555555555555, indexofsecondlastbytes,
                                                            beforeasciibytes); // the second last bytes (of two, three byte seq,
                                                                               // surrogates)
    secondlastbytes = _mm512_slli_epi16(secondlastbytes, 6);                   // shifted into position
    __m512i secondandlastbytes = _mm512_add_epi16(secondlastbytes, lastbytes);

    __mmask64 mask_thirdlastbytes = _kand_mask64(m34, 0x3fffffffffffffff); // bytes that could be third-last bytes
                                                                           // (LEAD34 sans wrap around)
    __m512i indexofthirdlastbytes = _mm512_add_epi16(mask_ffffffff,
                                                     indexofsecondlastbytes); // indices of the second last bytes
    __m512i thirdlastbyte = _mm512_maskz_mov_epi8(mask_thirdlastbytes,
                                                  clearedbytes); // only those that are the third last byte of a sequece
    __m512i thirdlastbytes = _mm512_maskz_permutexvar_epi8(0x5555555555555555, indexofthirdlastbytes,
                                                           thirdlastbyte); // the third last bytes (of three byte sequences, hi
                                                                           // surrogate)
    thirdlastbytes = _mm512_slli_epi16(thirdlastbytes, 12);                // shifted into position
    __m512i thirdsecondandlastbytes = _mm512_add_epi16(secondandlastbytes, thirdlastbytes);

    __mmask32 Mlo = _pext_u64(mp3, mend);
    __mmask32 Mhi = (Mlo >> 1);
    __m512i lo_surr_mask = _mm512_maskz_mov_epi16(Mlo,
                                                  mask_dc00dc00); // lo surr: 1101110000000000, other:  0000000000000000
    __m512i shifted4_thirdsecondandlastbytes = _mm512_srli_epi16(thirdsecondandlastbytes,
                                                                 4); // hi surr: 00000WVUTSRQPNML  vuts = WVUTS - 1
    __m512i tagged_lo_surrogates = _mm512_or_si512(thirdsecondandlastbytes,
                                                   lo_surr_mask); // lo surr: 110111KJHGFEDCBA, other:  unchanged
    __m512i Wout = _mm512_mask_add_epi16(tagged_lo_surrogates, Mhi, shifted4_thirdsecondandlastbytes,
                                         mask_d7c0d7c0); // hi sur: 110110vutsRQPNML, other:  unchanged
    // the elements of Wout excluding the last element if it happens to be a high surrogate:
    __mmask32 Mout = ~(Mhi & 0x80000000);
    //  the locations of the last byte of each sequence that has been processed into a word :
    //  (It might not be needed to AND with b.)
    __mmask64 mprocessed = (tail == SIMDUTF_FULL) ? _pdep_u64(Mout, mend) : _pdep_u64(Mout, _kand_mask64(mend, b)); // we adjust mend at the end of the output.

    int nout = _mm_popcnt_u64(mprocessed);
    int nin = 64 - _lzcnt_u64(mprocessed);
    // Encodings out of range...
    {
      // the location of 3-byte sequence start bytes in the input
      __mmask64 m3 = m34 & (b ^ m4);
      // words in Wout corresponding to 3-byte sequences.
      __mmask64 M3 = _pext_u64(m3 << 2, mend);
      __m512i mask_08000800 = _mm512_set1_epi32(0x08000800);
      __mmask32 Msmall800 = _mm512_mask_cmplt_epi16_mask(M3, Wout, mask_08000800);
      __m512i mask_d800d800 = _mm512_set1_epi32(0xd800d800);
      __m512i Moutminusd800 = _mm512_sub_epi16(Wout, mask_d800d800);
      __mmask32 M3s = _mm512_mask_cmplt_epi16_mask(M3, Moutminusd800, mask_08000800);
      __m512i mask_04000400 = _mm512_set1_epi32(0x04000400);
      __mmask32 M4s = _mm512_mask_cmpge_epi16_mask(Mhi, Moutminusd800, mask_04000400);

      if (!_kortestz_mask32_u8(M4s, _kor_mask32(Msmall800, M3s))) {
        // Encodings out of range
        return __tzcnt_u64(_pdep_u64(_kor_mask64(m1234, mp3), _kor_mask32(M4s, _kor_mask32(Msmall800, M3s))));
      }
    }

    _mm512_mask_storeu_epi16(out, ((uint64_t(1) << nout) - 1), Wout);
    out += nout;
    in += nin;
    return SIMDUTF_OK; // ok
  }

  // Fast path 2: all ASCII or 2 byte
  // on top of -0xc0 we substract -2 which we get back later of the
  // continuation byte tags
  __m512i leading2byte = _mm512_maskz_sub_epi8(m234, input, mask_c2c2c2c2);
  __mmask64 leading = tail == (tail == SIMDUTF_FULL) ? _kor_mask64(m1, m234) : _kand_mask64(_kor_mask64(m1, m234), b); // first bytes of each sequence
  __mmask64 continuation_or_ascii = (tail == SIMDUTF_FULL) ? _knot_mask64(m234) : _kand_mask64(_knot_mask64(m234), b);
  if ((m234 << 1) != (b ^ leading)) {
    // two byte without continuation
    // continuation bytes at (b ^ leading), they should be at (m234 << 1),
    // so if (b ^ leading) &~ (m234 << 1) is non zero...
    // there is a continuation byte present where there should not be one
    int err1 = __tzcnt_u64((m234 << 1) ^ (b ^ leading));
    if (((b ^ leading) & ~(m234 << 1)) != 0) {
      return err1;
    }
    // err1 will point at a missing continuation byte,
    // and the leading byte should be prior to it.
    uint64_t mpre = (uint64_t(1) << err1) - 1;
    //  lead byte that is missing a continuation byte
    uint64_t missing = (mpre & m234);
    return 64 - _lzcnt_u64(missing) - 1;
  }
  __m512i lead = _mm512_maskz_compress_epi8(leading, leading2byte);          // will contain zero for ascii, and the data
  lead = _mm512_cvtepu8_epi16(_mm512_castsi512_si256(lead));                 // ... zero extended into words
  __m512i follow = _mm512_maskz_compress_epi8(continuation_or_ascii, input); // the last bytes of each sequence
  follow = _mm512_cvtepu8_epi16(_mm512_castsi512_si256(follow));             // ... zero extended into words
  lead = _mm512_slli_epi16(lead, 6);                                         // shifted into position
  __m512i final = _mm512_add_epi16(follow, lead);                            // combining lead and follow
  int nout, nin;
  if (tail == SIMDUTF_FULL) {
    // Next part is UTF-16 specific and can be generalized to UTF-32.
    _mm512_storeu_epi16(out, final);
    nout = 32;
    nin = 64 - _lzcnt_u64(_pdep_u64(0xFFFFFFFF, continuation_or_ascii));
  } else {
    nout = _mm_popcnt_u64(_pdep_u64(0xFFFFFFFF, leading));
    nin = 64 - _lzcnt_u64(_pdep_u64(0xFFFFFFFF, continuation_or_ascii));
    _mm512_mask_storeu_epi16(out, ((uint64_t(1) << nout) - 1), final);
  }
  out += nout; // UTF-8 to UTF-16 is only expansionary in this case.
  // computing the consumed input is more fun:
  in += nin;
  return SIMDUTF_OK; // we are fine.
}

std::pair<const char *, char16_t *> new_convert_utf8_to_utf16(const char *in, size_t len, char16_t *out) {
  const char *const final_in = in + len;

  // main loop
  while (in + 64 <= final_in) {
    int result = process_block_utf8_to_utf16<SIMDUTF_FULL>(in, out, final_in - in);
    if (result != -1) {
      return std::make_pair(in, nullptr);
    }
  }
  // Need to handle the tail.
  // We might need to call it more than once.
  while (in < final_in) {
    int result = process_block_utf8_to_utf16<SIMDUTF_TAIL>(in, out, final_in - in);
    if (result != -1) {
      return std::make_pair(in, nullptr);
    }
  }
  return std::make_pair(in, out);
}

simdutf_warn_unused size_t implementation::convert_utf8_to_utf16(const char* buf, size_t len, char16_t* utf16_output) const noexcept {
  utf8_to_utf16_result ret = new_convert_utf8_to_utf16(buf, len, utf16_output);//icelake::validating_utf8_to_fixed_length<char16_t>(buf, len, utf16_output);
  if (ret.second == nullptr)
    return 0;

  return ret.second - utf16_output;
}

simdutf_warn_unused size_t implementation::convert_valid_utf8_to_utf16(const char* buf, size_t len, char16_t* utf16_output) const noexcept {
  utf8_to_utf16_result ret = icelake::valid_utf8_to_fixed_length<char16_t>(buf, len, utf16_output);
  size_t saved_bytes = ret.second - utf16_output;
  const char* end = buf + len;
  if (ret.first == end) {
    return saved_bytes;
  }

  // Note: AVX512 procedure looks up 4 bytes forward, and
  //       correctly converts multi-byte chars even if their
  //       continuation bytes lie outsiede 16-byte window.
  //       It meas, we have to skip continuation bytes from
  //       the beginning ret.first, as they were already consumed.
  while (ret.first != end && ((uint8_t(*ret.first) & 0xc0) == 0x80)) {
      ret.first += 1;
  }

  if (ret.first != end) {
    const size_t scalar_saved_bytes = scalar::utf8_to_utf16::convert_valid(
                                        ret.first, len - (ret.first - buf), ret.second);
    if (scalar_saved_bytes == 0) { return 0; }
    saved_bytes += scalar_saved_bytes;
  }

  return saved_bytes;
}

simdutf_warn_unused size_t implementation::convert_utf8_to_utf32(const char* buf, size_t len, char32_t* utf32_out) const noexcept {
  uint32_t * utf32_output = reinterpret_cast<uint32_t *>(utf32_out);
  utf8_to_utf32_result ret = icelake::validating_utf8_to_fixed_length<uint32_t>(buf, len, utf32_output);
  if (ret.second == nullptr)
    return 0;

  size_t saved_bytes = ret.second - utf32_output;
  const char* end = buf + len;
  if (ret.first == end) {
    return saved_bytes;
  }

  // Note: the AVX512 procedure looks up 4 bytes forward, and
  //       correctly converts multi-byte chars even if their
  //       continuation bytes lie outside 16-byte window.
  //       It means, we have to skip continuation bytes from
  //       the beginning ret.first, as they were already consumed.
  while (ret.first != end and ((uint8_t(*ret.first) & 0xc0) == 0x80)) {
      ret.first += 1;
  }

  if (ret.first != end) {
    const size_t scalar_saved_bytes = scalar::utf8_to_utf32::convert(
                                        ret.first, len - (ret.first - buf), utf32_out + saved_bytes);
    if (scalar_saved_bytes == 0) { return 0; }
    saved_bytes += scalar_saved_bytes;
  }

  return saved_bytes;
}

simdutf_warn_unused size_t implementation::convert_valid_utf8_to_utf32(const char* buf, size_t len, char32_t* utf32_out) const noexcept {
  uint32_t * utf32_output = reinterpret_cast<uint32_t *>(utf32_out);
  utf8_to_utf32_result ret = icelake::valid_utf8_to_fixed_length<uint32_t>(buf, len, utf32_output);
  size_t saved_bytes = ret.second - utf32_output;
  const char* end = buf + len;
  if (ret.first == end) {
    return saved_bytes;
  }

  // Note: AVX512 procedure looks up 4 bytes forward, and
  //       correctly converts multi-byte chars even if their
  //       continuation bytes lie outsiede 16-byte window.
  //       It meas, we have to skip continuation bytes from
  //       the beginning ret.first, as they were already consumed.
  while (ret.first != end && ((uint8_t(*ret.first) & 0xc0) == 0x80)) {
      ret.first += 1;
  }

  if (ret.first != end) {
    const size_t scalar_saved_bytes = scalar::utf8_to_utf32::convert_valid(
                                        ret.first, len - (ret.first - buf), utf32_out + saved_bytes);
    if (scalar_saved_bytes == 0) { return 0; }
    saved_bytes += scalar_saved_bytes;
  }

  return saved_bytes;
}

simdutf_warn_unused size_t implementation::convert_utf16_to_utf8(const char16_t* buf, size_t len, char* utf8_output) const noexcept {
  return scalar::utf16_to_utf8::convert(buf, len, utf8_output);
}

simdutf_warn_unused size_t implementation::convert_valid_utf16_to_utf8(const char16_t* buf, size_t len, char* utf8_output) const noexcept {
  return scalar::utf16_to_utf8::convert_valid(buf, len, utf8_output);
}


simdutf_warn_unused size_t implementation::convert_utf32_to_utf8(const char32_t* buf, size_t len, char* utf8_output) const noexcept {
  return scalar::utf32_to_utf8::convert(buf, len, utf8_output);
}

simdutf_warn_unused size_t implementation::convert_valid_utf32_to_utf8(const char32_t* buf, size_t len, char* utf8_output) const noexcept {
  return scalar::utf32_to_utf8::convert_valid(buf, len, utf8_output);
}

simdutf_warn_unused size_t implementation::convert_utf32_to_utf16(const char32_t* buf, size_t len, char16_t* utf16_output) const noexcept {
  return scalar::utf32_to_utf16::convert(buf, len, utf16_output);
}

simdutf_warn_unused size_t implementation::convert_valid_utf32_to_utf16(const char32_t* buf, size_t len, char16_t* utf16_output) const noexcept {
  return scalar::utf32_to_utf16::convert_valid(buf, len, utf16_output);
}

simdutf_warn_unused size_t implementation::convert_utf16_to_utf32(const char16_t* buf, size_t len, char32_t* utf32_output) const noexcept {
  std::pair<const char16_t*, char32_t*> ret = icelake::convert_utf16_to_utf32(buf, len, utf32_output);
  if (ret.first == nullptr) { return 0; }
  size_t saved_bytes = ret.second - utf32_output;
  if (ret.first != buf + len) {
    const size_t scalar_saved_bytes = scalar::utf16_to_utf32::convert(
                                        ret.first, len - (ret.first - buf), ret.second);
    if (scalar_saved_bytes == 0) { return 0; }
    saved_bytes += scalar_saved_bytes;
  }
  return saved_bytes;
}

simdutf_warn_unused size_t implementation::convert_valid_utf16_to_utf32(const char16_t* buf, size_t len, char32_t* utf32_output) const noexcept {
  return scalar::utf16_to_utf32::convert_valid(buf, len, utf32_output);
}

simdutf_warn_unused size_t implementation::count_utf16(const char16_t * input, size_t length) const noexcept {
  const char16_t* end = length >= 32 ? input + length - 32 : nullptr;
  const char16_t* ptr = input;

  const __m512i low = _mm512_set1_epi16((uint16_t)0xdc00);
  const __m512i high = _mm512_set1_epi16((uint16_t)0xdfff);

  size_t count{0};

  while (ptr <= end) {
    __m512i utf16 = _mm512_loadu_si512((const __m512i*)ptr);
    ptr += 32;
    uint64_t not_high_surrogate = static_cast<uint64_t>(_mm512_cmpgt_epu16_mask(utf16, high) | _mm512_cmplt_epu16_mask(utf16, low));
    count += count_ones(not_high_surrogate);
  }

  return count + scalar::utf16::count_code_points(ptr, length - (ptr - input));
}

simdutf_warn_unused size_t implementation::count_utf8(const char * input, size_t length) const noexcept {
  const char* end = length >= 64 ? input + length - 64 : nullptr;
  const char* ptr = input;

  const __m512i continuation = _mm512_set1_epi8(char(0b10111111));

  size_t count{0};

  while (ptr <= end) {
    __m512i utf8 = _mm512_loadu_si512((const __m512i*)ptr);
    ptr += 64;
    uint64_t continuation_bitmask = static_cast<uint64_t>(_mm512_cmple_epi8_mask(utf8, continuation));
    count += 64 - count_ones(continuation_bitmask);
  }

  return count + scalar::utf8::count_code_points(ptr, length - (ptr - input));
}

simdutf_warn_unused size_t implementation::utf8_length_from_utf16(const char16_t * input, size_t length) const noexcept {
  const char16_t* end = length >= 32 ? input + length - 32 : nullptr;
  const char16_t* ptr = input;

  const __m512i v_007f = _mm512_set1_epi16((uint16_t)0x007f);
  const __m512i v_07ff = _mm512_set1_epi16((uint16_t)0x07ff);
  const __m512i v_dfff = _mm512_set1_epi16((uint16_t)0xdfff);
  const __m512i v_d800 = _mm512_set1_epi16((uint16_t)0xd800);

  size_t count{0};

  while (ptr <= end) {
    __m512i utf16 = _mm512_loadu_si512((const __m512i*)ptr);
    ptr += 32;
    __mmask32 ascii_bitmask = _mm512_cmple_epu16_mask(utf16, v_007f);
    __mmask32 two_bytes_bitmask = _mm512_mask_cmple_epu16_mask(~ascii_bitmask, utf16, v_07ff);
    __mmask32 not_one_two_bytes = ~(ascii_bitmask | two_bytes_bitmask);
    __mmask32 surrogates_bitmask = _mm512_mask_cmple_epu16_mask(not_one_two_bytes, utf16, v_dfff) & _mm512_mask_cmpge_epu16_mask(not_one_two_bytes, utf16, v_d800);

    size_t ascii_count = count_ones(ascii_bitmask);
    size_t two_bytes_count = count_ones(two_bytes_bitmask);
    size_t surrogate_bytes_count = count_ones(surrogates_bitmask);
    size_t three_bytes_count = 32 - ascii_count - two_bytes_count - surrogate_bytes_count;

    count += ascii_count + 2*two_bytes_count + 3*three_bytes_count + 2*surrogate_bytes_count;
  }

  return count + scalar::utf16::utf8_length_from_utf16(ptr, length - (ptr - input));
}

simdutf_warn_unused size_t implementation::utf8_length_from_utf32(const char32_t * input, size_t length) const noexcept {
  const char32_t* end = length >= 16 ? input + length - 16 : nullptr;
  const char32_t* ptr = input;

  const __m512i v_0000_007f = _mm512_set1_epi32((uint32_t)0x7f);
  const __m512i v_0000_07ff = _mm512_set1_epi32((uint32_t)0x7ff);
  const __m512i v_0000_ffff = _mm512_set1_epi32((uint32_t)0x0000ffff);

  size_t count{0};

  while (ptr <= end) {
    __m512i utf32 = _mm512_loadu_si512((const __m512i*)ptr);
    ptr += 16;
    __mmask16 ascii_bitmask = _mm512_cmple_epu32_mask(utf32, v_0000_007f);
    __mmask16 two_bytes_bitmask = _mm512_mask_cmple_epu32_mask(_knot_mask16(ascii_bitmask), utf32, v_0000_07ff);
    __mmask16 three_bytes_bitmask = _mm512_mask_cmple_epu32_mask(_knot_mask16(_mm512_kor(ascii_bitmask, two_bytes_bitmask)), utf32, v_0000_ffff);

    size_t ascii_count = count_ones(ascii_bitmask);
    size_t two_bytes_count = count_ones(two_bytes_bitmask);
    size_t three_bytes_count = count_ones(three_bytes_bitmask);
    size_t four_bytes_count = 16 - ascii_count - two_bytes_count - three_bytes_count;
    count += ascii_count + 2*two_bytes_count + 3*three_bytes_count + 4*four_bytes_count;
  }

  return count + scalar::utf32::utf8_length_from_utf32(ptr, length - (ptr - input));
}

simdutf_warn_unused size_t implementation::utf16_length_from_utf8(const char * input, size_t length) const noexcept {
  const char* end = length >= 64 ? input + length - 64 : nullptr;
  const char* ptr = input;

  const __m512i continuation = _mm512_set1_epi8(char(0b10111111));
  const __m512i utf8_4bytes = _mm512_set1_epi8(char(0b11110000));

  size_t count{0};

  while (ptr <= end) {
    __m512i utf8 = _mm512_loadu_si512((const __m512i*)ptr);
    ptr += 64;
    uint64_t continuation_bitmask = static_cast<uint64_t>(_mm512_cmple_epi8_mask(utf8, continuation));
    uint64_t utf8_4bytes_bitmask = static_cast<uint64_t>(_mm512_cmpge_epu8_mask(utf8, utf8_4bytes));
    count += 64 + count_ones(utf8_4bytes_bitmask) - count_ones(continuation_bitmask);
  }

  return count + scalar::utf8::utf16_length_from_utf8(ptr, length - (ptr - input));
}

simdutf_warn_unused size_t implementation::utf32_length_from_utf16(const char16_t * input, size_t length) const noexcept {
  return implementation::count_utf16(input, length);
}

simdutf_warn_unused size_t implementation::utf16_length_from_utf32(const char32_t * input, size_t length) const noexcept {
  const char32_t* end = length >= 16 ? input + length - 16 : nullptr;
  const char32_t* ptr = input;

  const __m512i v_0000_ffff = _mm512_set1_epi32((uint32_t)0x0000ffff);

  size_t count{0};

  while (ptr <= end) {
    __m512i utf32 = _mm512_loadu_si512((const __m512i*)ptr);
    ptr += 16;
    __mmask16 surrogates_bitmask = _mm512_cmpgt_epu32_mask(utf32, v_0000_ffff);

    count += 16 + count_ones(surrogates_bitmask);
  }

  return count + scalar::utf32::utf16_length_from_utf32(ptr, length - (ptr - input));
}

simdutf_warn_unused size_t implementation::utf32_length_from_utf8(const char * input, size_t length) const noexcept {
  return implementation::count_utf8(input, length);
}

} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf

#include "simdutf/icelake/end.h"
