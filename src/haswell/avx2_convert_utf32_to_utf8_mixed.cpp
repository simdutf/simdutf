#include "simdutf/haswell/begin.h"

namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {

simdutf_really_inline uint8_t spread_four_bits(uint8_t bits) {
  bits = static_cast<uint8_t>((bits | (bits << 2)) & 0x33);
  return static_cast<uint8_t>((bits | (bits << 1)) & 0x55);
}

// Pack eight valid UTF-32 code units. Each group of four is compressed with a
// PSHUFB plan indexed by its two-bit-per-code-unit UTF-8 widths.
simdutf_really_inline char *avx2_pack_eight_utf32_to_utf8(__m256i input,
                                                          char *utf8_output) {
  const __m256i v_007f = _mm256_set1_epi32(0x007f);
  const __m256i v_07ff = _mm256_set1_epi32(0x07ff);
  const __m256i v_ffff = _mm256_set1_epi32(0xffff);
  const __m256i v_24 = _mm256_set1_epi32(24);
  const __m256i v_3f000000 =
      _mm256_set1_epi32(static_cast<int32_t>(0x3f000000u));
  const __m256i v_003f0000 = _mm256_set1_epi32(0x003f0000);
  const __m256i v_00003f00 = _mm256_set1_epi32(0x00003f00);
  const __m256i v_808080f0 =
      _mm256_set1_epi32(static_cast<int32_t>(0x808080f0u));
  const __m256i v_80808000 =
      _mm256_set1_epi32(static_cast<int32_t>(0x80808000u));
  const __m256i correction_table =
      _mm256_setr_epi8(static_cast<char>(0x80), 0x40, 0x60, 0, 0, 0, 0, 0, 0, 0,
                       0, 0, 0, 0, 0, 0, static_cast<char>(0x80), 0x40, 0x60, 0,
                       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

  // Each comparison is all ones in lanes that need the next UTF-8 byte. Their
  // negated sum is therefore the width minus one (from zero through three).
  const __m256i more_than_7f = _mm256_cmpgt_epi32(input, v_007f);
  const __m256i more_than_7ff = _mm256_cmpgt_epi32(input, v_07ff);
  const __m256i more_than_ffff = _mm256_cmpgt_epi32(input, v_ffff);
  const __m256i widths = _mm256_sub_epi32(
      _mm256_setzero_si256(),
      _mm256_add_epi32(_mm256_add_epi32(more_than_7f, more_than_7ff),
                       more_than_ffff));

  // Form a four-byte UTF-8 sequence in every 32-bit lane. For shorter
  // sequences, shift away unused leading continuation bytes and correct the
  // leading byte below.
  __m256i encoded = _mm256_or_si256(
      _mm256_or_si256(
          _mm256_and_si256(_mm256_slli_epi32(input, 24), v_3f000000),
          _mm256_and_si256(_mm256_slli_epi32(input, 10), v_003f0000)),
      _mm256_or_si256(_mm256_and_si256(_mm256_srli_epi32(input, 4), v_00003f00),
                      _mm256_srli_epi32(input, 18)));
  encoded = _mm256_or_si256(encoded, v_808080f0);

  const __m256i shifts = _mm256_sub_epi32(v_24, _mm256_slli_epi32(widths, 3));
  const __m256i correction = _mm256_shuffle_epi8(
      correction_table, _mm256_or_si256(widths, v_80808000));
  encoded = _mm256_xor_si256(_mm256_srlv_epi32(encoded, shifts), correction);
  // The generic four-byte representation carries six payload bits. Restore
  // the seventh ASCII bit by selecting the original lane for one-byte input.
  encoded = _mm256_blendv_epi8(input, encoded, more_than_7f);

  const uint8_t more_than_7f_mask = static_cast<uint8_t>(
      _mm256_movemask_ps(_mm256_castsi256_ps(more_than_7f)));
  const uint8_t more_than_7ff_mask = static_cast<uint8_t>(
      _mm256_movemask_ps(_mm256_castsi256_ps(more_than_7ff)));
  const uint8_t more_than_ffff_mask = static_cast<uint8_t>(
      _mm256_movemask_ps(_mm256_castsi256_ps(more_than_ffff)));
  const uint8_t low_width_bits = static_cast<uint8_t>(
      more_than_7f_mask ^ more_than_7ff_mask ^ more_than_ffff_mask);
  const uint8_t high_width_bits = more_than_7ff_mask;
  const uint8_t key0 =
      static_cast<uint8_t>(spread_four_bits(low_width_bits & 0x0f) |
                           (spread_four_bits(high_width_bits & 0x0f) << 1));
  const uint8_t key1 =
      static_cast<uint8_t>(spread_four_bits(low_width_bits >> 4) |
                           (spread_four_bits(high_width_bits >> 4) << 1));

  const uint8_t *row0 =
      simdutf::tables::utf32_to_utf8::pack_1_2_3_4_utf8_bytes.rows[key0];
  const uint8_t *row1 =
      simdutf::tables::utf32_to_utf8::pack_1_2_3_4_utf8_bytes.rows[key1];
  const __m128i packed0 = _mm_shuffle_epi8(
      _mm256_castsi256_si128(encoded),
      _mm_loadu_si128(reinterpret_cast<const __m128i *>(row0 + 1)));
  const __m128i packed1 = _mm_shuffle_epi8(
      _mm256_extracti128_si256(encoded, 1),
      _mm_loadu_si128(reinterpret_cast<const __m128i *>(row1 + 1)));

  _mm_storeu_si128(reinterpret_cast<__m128i *>(utf8_output), packed0);
  utf8_output += row0[0];
  _mm_storeu_si128(reinterpret_cast<__m128i *>(utf8_output), packed1);
  return utf8_output + row1[0];
}

simdutf_really_inline char *
avx2_pack_eight_four_byte_utf32_to_utf8(__m256i input, char *utf8_output) {
  const __m256i v_3f000000 =
      _mm256_set1_epi32(static_cast<int32_t>(0x3f000000u));
  const __m256i v_003f0000 = _mm256_set1_epi32(0x003f0000);
  const __m256i v_00003f00 = _mm256_set1_epi32(0x00003f00);
  const __m256i v_808080f0 =
      _mm256_set1_epi32(static_cast<int32_t>(0x808080f0u));
  const __m256i encoded = _mm256_or_si256(
      _mm256_or_si256(
          _mm256_and_si256(_mm256_slli_epi32(input, 24), v_3f000000),
          _mm256_and_si256(_mm256_slli_epi32(input, 10), v_003f0000)),
      _mm256_or_si256(
          _mm256_and_si256(_mm256_srli_epi32(input, 4), v_00003f00),
          _mm256_or_si256(_mm256_srli_epi32(input, 18), v_808080f0)));
  _mm256_storeu_si256(reinterpret_cast<__m256i *>(utf8_output), encoded);
  return utf8_output + 32;
}

// Once a non-BMP block has been encountered, process the rest of the vector
// region with a two-bit width plan per input code unit. Invalid blocks are left
// untouched for the scalar resume path, which reports the exact first error.
simdutf_really_inline bool avx2_all_non_bmp_block(const char32_t *input) {
  const __m256i in =
      _mm256_loadu_si256(reinterpret_cast<const __m256i *>(input));
  const __m256i nextin =
      _mm256_loadu_si256(reinterpret_cast<const __m256i *>(input) + 1);
  const __m256i v_ffff = _mm256_set1_epi32(0xffff);
  const __m256i all_non_bmp =
      _mm256_cmpgt_epi32(_mm256_min_epu32(in, nextin), v_ffff);
  return static_cast<uint32_t>(_mm256_movemask_epi8(all_non_bmp)) == 0xffffffff;
}

std::pair<result, char *> avx2_convert_mixed_utf32_to_utf8_with_errors(
    const char32_t *buf, size_t len, char *utf8_output, size_t input_offset);
std::pair<result, char *> avx2_convert_four_byte_utf32_to_utf8_with_errors(
    const char32_t *buf, size_t len, char *utf8_output, size_t input_offset);

#if !defined(SIMDUTF_REGULAR_VISUAL_STUDIO)
__attribute__((cold))
#endif
simdutf_never_inline avx2_utf32_progress
avx2_pack_mixed_utf32_to_utf8(const char32_t *buf, size_t len,
                              char *utf8_output) {
  // Both packers reserve a 12-code-point tail for their wide stores.
  if (len < 16 + 12) {
    return {0, 0};
  }
  const std::pair<result, char *> ret =
      avx2_all_non_bmp_block(buf)
          ? avx2_convert_four_byte_utf32_to_utf8_with_errors(buf, len,
                                                             utf8_output, 0)
          : avx2_convert_mixed_utf32_to_utf8_with_errors(buf, len, utf8_output,
                                                         0);
  return {ret.first.count, size_t(ret.second - utf8_output)};
}

simdutf_never_inline std::pair<result, char *>
avx2_convert_mixed_utf32_to_utf8_with_errors(const char32_t *buf, size_t len,
                                             char *utf8_output,
                                             size_t input_offset) {
  const char32_t *const start = buf;
  const char32_t *const end = buf + len;
  const __m256i v_10ffff = _mm256_set1_epi32(0x10ffff);
  const __m256i v_fffff800 =
      _mm256_set1_epi32(static_cast<int32_t>(0xfffff800u));
  const __m256i v_d800 = _mm256_set1_epi32(0xd800);
  // A packed four-code-unit store can write 16 bytes even when its useful
  // prefix is shorter. Four useful bytes plus the 12-code-unit tail suffice.
  const size_t safety_margin = 12;

  while (end - buf >= std::ptrdiff_t(16 + safety_margin)) {
    const __m256i in =
        _mm256_loadu_si256(reinterpret_cast<const __m256i *>(buf));
    const __m256i nextin =
        _mm256_loadu_si256(reinterpret_cast<const __m256i *>(buf) + 1);
    const __m256i max_input =
        _mm256_max_epu32(_mm256_max_epu32(in, nextin), v_10ffff);
    if (static_cast<uint32_t>(_mm256_movemask_epi8(
            _mm256_cmpeq_epi32(max_input, v_10ffff))) != 0xffffffff) {
      break;
    }

    const __m256i surrogate_bytemask = _mm256_or_si256(
        _mm256_cmpeq_epi32(_mm256_and_si256(in, v_fffff800), v_d800),
        _mm256_cmpeq_epi32(_mm256_and_si256(nextin, v_fffff800), v_d800));
    if (static_cast<uint32_t>(_mm256_movemask_epi8(surrogate_bytemask)) != 0) {
      break;
    }

    // Each packed four-code-unit result uses a 16-byte store. Validate the
    // twelve following code units before those stores so their padding is
    // backed by output that is known to exist even when the next block fails.
    const __m256i tail =
        _mm256_loadu_si256(reinterpret_cast<const __m256i *>(buf + 12));
    const __m256i tailnext =
        _mm256_loadu_si256(reinterpret_cast<const __m256i *>(buf + 20));
    const __m256i tail_max_input =
        _mm256_max_epu32(_mm256_max_epu32(tail, tailnext), v_10ffff);
    if (static_cast<uint32_t>(_mm256_movemask_epi8(
            _mm256_cmpeq_epi32(tail_max_input, v_10ffff))) != 0xffffffff) {
      break;
    }
    const __m256i tail_surrogate_bytemask = _mm256_or_si256(
        _mm256_cmpeq_epi32(_mm256_and_si256(tail, v_fffff800), v_d800),
        _mm256_cmpeq_epi32(_mm256_and_si256(tailnext, v_fffff800), v_d800));
    if (static_cast<uint32_t>(_mm256_movemask_epi8(tail_surrogate_bytemask)) !=
        0) {
      break;
    }

    utf8_output = avx2_pack_eight_utf32_to_utf8(in, utf8_output);
    utf8_output = avx2_pack_eight_utf32_to_utf8(nextin, utf8_output);
    buf += 16;
  }

  return std::make_pair(
      result(error_code::SUCCESS, input_offset + size_t(buf - start)),
      utf8_output);
}

simdutf_never_inline std::pair<result, char *>
avx2_convert_four_byte_utf32_to_utf8_with_errors(const char32_t *buf,
                                                 size_t len, char *utf8_output,
                                                 size_t input_offset) {
  const char32_t *const start = buf;
  const char32_t *const end = buf + len;
  const __m256i v_10ffff = _mm256_set1_epi32(0x10ffff);
  const __m256i v_ffff = _mm256_set1_epi32(0xffff);
  const __m256i v_fffff800 =
      _mm256_set1_epi32(static_cast<int32_t>(0xfffff800u));
  const __m256i v_d800 = _mm256_set1_epi32(0xd800);
  const size_t safety_margin = 12;

  while (end - buf >= std::ptrdiff_t(16 + safety_margin)) {
    const __m256i in =
        _mm256_loadu_si256(reinterpret_cast<const __m256i *>(buf));
    const __m256i nextin =
        _mm256_loadu_si256(reinterpret_cast<const __m256i *>(buf) + 1);
    const __m256i max_input =
        _mm256_max_epu32(_mm256_max_epu32(in, nextin), v_10ffff);
    if (static_cast<uint32_t>(_mm256_movemask_epi8(
            _mm256_cmpeq_epi32(max_input, v_10ffff))) != 0xffffffff) {
      break;
    }

    const __m256i surrogate_bytemask = _mm256_or_si256(
        _mm256_cmpeq_epi32(_mm256_and_si256(in, v_fffff800), v_d800),
        _mm256_cmpeq_epi32(_mm256_and_si256(nextin, v_fffff800), v_d800));
    if (static_cast<uint32_t>(_mm256_movemask_epi8(surrogate_bytemask)) != 0) {
      break;
    }

    const __m256i all_non_bmp =
        _mm256_cmpgt_epi32(_mm256_min_epu32(in, nextin), v_ffff);
    if (static_cast<uint32_t>(_mm256_movemask_epi8(all_non_bmp)) !=
        0xffffffff) {
      return avx2_convert_mixed_utf32_to_utf8_with_errors(
          buf, size_t(end - buf), utf8_output,
          input_offset + size_t(buf - start));
    }

    utf8_output = avx2_pack_eight_four_byte_utf32_to_utf8(in, utf8_output);
    utf8_output = avx2_pack_eight_four_byte_utf32_to_utf8(nextin, utf8_output);
    buf += 16;
  }

  return std::make_pair(
      result(error_code::SUCCESS, input_offset + size_t(buf - start)),
      utf8_output);
}

} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf

#include "simdutf/haswell/end.h"
