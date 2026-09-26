#include "simdutf/fallback/begin.h"

#if defined(SIMDUTF_IS_X86_64) && !defined(SIMDUTF_REGULAR_VISUAL_STUDIO)
  #include <immintrin.h>
#endif

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32
  #if defined(SIMDUTF_IS_X86_64) && !defined(SIMDUTF_REGULAR_VISUAL_STUDIO)
SIMDUTF_TARGET_REGION("ssse3,sse4.1")
namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {

struct utf32_packed_progress {
  size_t input;
  size_t output;
};

constexpr size_t utf32_code_units_per_pack = 4;
constexpr size_t utf8_packed_store_bytes = 16;
constexpr size_t utf32_pack_groups_per_batch = 16;
// A four-code-unit result has at least four useful bytes, so twelve following
// valid code units back the remaining bytes of its 16-byte wide store.
constexpr size_t utf32_pack_lookahead =
    utf8_packed_store_bytes - utf32_code_units_per_pack;

simdutf_really_inline uint8_t spread_four_bits(uint8_t bits) {
  bits = static_cast<uint8_t>((bits | (bits << 2)) & 0x33);
  return static_cast<uint8_t>((bits | (bits << 1)) & 0x55);
}

bool sse41_valid_utf32_blocks(const char32_t *buf, size_t block_count) {
  const __m128i v_10ffff = _mm_set1_epi32(0x10ffff);
  const __m128i v_fffff800 = _mm_set1_epi32(static_cast<int32_t>(0xfffff800u));
  const __m128i v_d800 = _mm_set1_epi32(0xd800);
  __m128i maximum = v_10ffff;
  __m128i surrogate = _mm_setzero_si128();
  for (size_t block = 0; block < block_count; block++) {
    const __m128i input =
        _mm_loadu_si128(reinterpret_cast<const __m128i *>(buf + block * 4));
    maximum = _mm_max_epu32(maximum, input);
    surrogate = _mm_or_si128(
        surrogate, _mm_cmpeq_epi32(_mm_and_si128(input, v_fffff800), v_d800));
  }
  return static_cast<uint32_t>(
             _mm_movemask_epi8(_mm_cmpeq_epi32(maximum, v_10ffff))) == 0xffff &&
         _mm_movemask_epi8(surrogate) == 0;
}

utf32_packed_progress sse41_pack_utf32_to_utf8(const char32_t *buf, size_t len,
                                               char *utf8_output) {
  const __m128i v_007f = _mm_set1_epi32(0x007f);
  const __m128i v_07ff = _mm_set1_epi32(0x07ff);
  const __m128i v_ffff = _mm_set1_epi32(0xffff);
  const __m128i v_3f000000 = _mm_set1_epi32(static_cast<int32_t>(0x3f000000u));
  const __m128i v_003f0000 = _mm_set1_epi32(0x003f0000);
  const __m128i v_00003f00 = _mm_set1_epi32(0x00003f00);
  const __m128i v_808080f0 = _mm_set1_epi32(static_cast<int32_t>(0x808080f0u));
  const __m128i v_40 = _mm_set1_epi32(0x40);
  const __m128i v_60 = _mm_set1_epi32(0x60);
  size_t pos = 0;
  char *const output_start = utf8_output;
  while (pos + utf32_pack_groups_per_batch * utf32_code_units_per_pack +
             utf32_pack_lookahead <=
         len) {
    // Validate the whole batch and its lookahead before the first wide store.
    // If it contains an error, scalar code resumes at this batch and preserves
    // the exact first-error index and output prefix.
    if (!sse41_valid_utf32_blocks(
            buf + pos, utf32_pack_groups_per_batch +
                           utf32_pack_lookahead / utf32_code_units_per_pack)) {
      break;
    }
    for (size_t group = 0; group < utf32_pack_groups_per_batch; group++) {
      const __m128i input =
          _mm_loadu_si128(reinterpret_cast<const __m128i *>(buf + pos));
      // The three comparison masks both classify each lane and form the two
      // bits per lane used to select a compacting PSHUFB row below.
      const __m128i more_than_7f = _mm_cmpgt_epi32(input, v_007f);
      const __m128i more_than_7ff = _mm_cmpgt_epi32(input, v_07ff);
      const __m128i more_than_ffff = _mm_cmpgt_epi32(input, v_ffff);
      const __m128i four_byte = _mm_or_si128(
          _mm_or_si128(_mm_and_si128(_mm_slli_epi32(input, 24), v_3f000000),
                       _mm_and_si128(_mm_slli_epi32(input, 10), v_003f0000)),
          _mm_or_si128(_mm_and_si128(_mm_srli_epi32(input, 4), v_00003f00),
                       _mm_srli_epi32(input, 18)));
      const __m128i four_byte_with_prefix = _mm_or_si128(four_byte, v_808080f0);
      __m128i encoded = _mm_blendv_epi8(
          input, _mm_xor_si128(_mm_srli_epi32(four_byte_with_prefix, 16), v_40),
          more_than_7f);
      encoded = _mm_blendv_epi8(
          encoded,
          _mm_xor_si128(_mm_srli_epi32(four_byte_with_prefix, 8), v_60),
          more_than_7ff);
      encoded = _mm_blendv_epi8(encoded, four_byte_with_prefix, more_than_ffff);

      const uint8_t more_than_7f_mask =
          static_cast<uint8_t>(_mm_movemask_ps(_mm_castsi128_ps(more_than_7f)));
      const uint8_t more_than_7ff_mask = static_cast<uint8_t>(
          _mm_movemask_ps(_mm_castsi128_ps(more_than_7ff)));
      const uint8_t more_than_ffff_mask = static_cast<uint8_t>(
          _mm_movemask_ps(_mm_castsi128_ps(more_than_ffff)));
      const uint8_t low_width_bits = static_cast<uint8_t>(
          more_than_7f_mask ^ more_than_7ff_mask ^ more_than_ffff_mask);
      const uint8_t high_width_bits = more_than_7ff_mask;
      const uint8_t key =
          static_cast<uint8_t>(spread_four_bits(low_width_bits) |
                               (spread_four_bits(high_width_bits) << 1));
      const uint8_t *row =
          simdutf::tables::utf32_to_utf8::pack_1_2_3_4_utf8_bytes.rows[key];
      const __m128i packed = _mm_shuffle_epi8(
          encoded, _mm_loadu_si128(reinterpret_cast<const __m128i *>(row + 1)));
      _mm_storeu_si128(reinterpret_cast<__m128i *>(utf8_output), packed);
      utf8_output += row[0];
      pos += 4;
    }
  }
  return {pos, size_t(utf8_output - output_start)};
}

} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf
SIMDUTF_UNTARGET_REGION
  #endif
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32

namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32
namespace {

struct utf32_ascii_progress {
  size_t input;
  size_t output;
};

utf32_ascii_progress copy_utf32_ascii_prefix(const char32_t *buf, size_t len,
                                             char *utf8_output) {
  size_t pos = 0;
  char *const output_start = utf8_output;
  while (pos + 4 <= len) {
    const uint32_t word0 = static_cast<uint32_t>(buf[pos]);
    const uint32_t word1 = static_cast<uint32_t>(buf[pos + 1]);
    const uint32_t word2 = static_cast<uint32_t>(buf[pos + 2]);
    const uint32_t word3 = static_cast<uint32_t>(buf[pos + 3]);
    if (((word0 | word1 | word2 | word3) & 0xffffff80) != 0) {
      break;
    }
    utf8_output[0] = static_cast<char>(word0);
    utf8_output[1] = static_cast<char>(word1);
    utf8_output[2] = static_cast<char>(word2);
    utf8_output[3] = static_cast<char>(word3);
    utf8_output += 4;
    pos += 4;
  }
  return {pos, size_t(utf8_output - output_start)};
}

} // unnamed namespace
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32
  #if defined(SIMDUTF_IS_X86_64) && !defined(SIMDUTF_REGULAR_VISUAL_STUDIO)
simdutf_never_inline bool fallback_sse41_supported() {
  // Keep this cache constant-initialized: the no-libc++ build cannot use the
  // guard helpers generated for dynamically initialized function statics.
  static uint32_t cached = 0;
  uint32_t state = __atomic_load_n(&cached, __ATOMIC_ACQUIRE);
  if (state == 0) {
    const uint32_t detected = (internal::detect_supported_architectures() &
                               internal::instruction_set::SSE42) != 0
                                  ? 2
                                  : 1;
    uint32_t expected = 0;
    __atomic_compare_exchange_n(&cached, &expected, detected, false,
                                __ATOMIC_RELEASE, __ATOMIC_ACQUIRE);
    state = __atomic_load_n(&cached, __ATOMIC_ACQUIRE);
  }
  return state == 2;
}
  #endif
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32

#if SIMDUTF_FEATURE_DETECT_ENCODING
simdutf_warn_unused int
implementation::detect_encodings(const char *input,
                                 size_t length) const noexcept {
  // If there is a BOM, then we trust it.
  auto bom_encoding = simdutf::BOM::check_bom(input, length);
  if (bom_encoding != encoding_type::unspecified) {
    return bom_encoding;
  }
  int out = 0;
  // todo: reimplement as a one-pass algorithm.
  if (validate_utf8(input, length)) {
    out |= encoding_type::UTF8;
  }
  if ((length % 2) == 0) {
    if (validate_utf16le(reinterpret_cast<const char16_t *>(input),
                         length / 2)) {
      out |= encoding_type::UTF16_LE;
    }
  }
  if ((length % 4) == 0) {
    if (validate_utf32(reinterpret_cast<const char32_t *>(input), length / 4)) {
      out |= encoding_type::UTF32_LE;
    }
  }
  return out;
}
#endif // SIMDUTF_FEATURE_DETECT_ENCODING

#if SIMDUTF_FEATURE_UTF8 || SIMDUTF_FEATURE_DETECT_ENCODING
simdutf_warn_unused bool
implementation::validate_utf8(const char *buf, size_t len) const noexcept {
  return scalar::utf8::validate(buf, len);
}
#endif // SIMDUTF_FEATURE_UTF8 || SIMDUTF_FEATURE_DETECT_ENCODING

#if SIMDUTF_FEATURE_UTF8
simdutf_warn_unused result implementation::validate_utf8_with_errors(
    const char *buf, size_t len) const noexcept {
  return scalar::utf8::validate_with_errors(buf, len);
}
#endif // SIMDUTF_FEATURE_UTF8

#if SIMDUTF_FEATURE_ASCII
simdutf_warn_unused bool
implementation::validate_ascii(const char *buf, size_t len) const noexcept {
  return scalar::ascii::validate(buf, len);
}

simdutf_warn_unused result implementation::validate_ascii_with_errors(
    const char *buf, size_t len) const noexcept {
  return scalar::ascii::validate_with_errors(buf, len);
}
#endif // SIMDUTF_FEATURE_ASCII

#if SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_ASCII
simdutf_warn_unused bool
implementation::validate_utf16le_as_ascii(const char16_t *buf,
                                          size_t len) const noexcept {
  return scalar::utf16::validate_as_ascii<endianness::LITTLE>(buf, len);
}

simdutf_warn_unused bool
implementation::validate_utf16be_as_ascii(const char16_t *buf,
                                          size_t len) const noexcept {
  return scalar::utf16::validate_as_ascii<endianness::BIG>(buf, len);
}
#endif // SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_ASCII
#if SIMDUTF_FEATURE_UTF16 || SIMDUTF_FEATURE_DETECT_ENCODING
simdutf_warn_unused bool
implementation::validate_utf16le(const char16_t *buf,
                                 size_t len) const noexcept {
  return scalar::utf16::validate<endianness::LITTLE>(buf, len);
}
#endif // SIMDUTF_FEATURE_UTF16 || SIMDUTF_FEATURE_DETECT_ENCODING

#if SIMDUTF_FEATURE_UTF16
simdutf_warn_unused bool
implementation::validate_utf16be(const char16_t *buf,
                                 size_t len) const noexcept {
  return scalar::utf16::validate<endianness::BIG>(buf, len);
}

simdutf_warn_unused result implementation::validate_utf16le_with_errors(
    const char16_t *buf, size_t len) const noexcept {
  return scalar::utf16::validate_with_errors<endianness::LITTLE>(buf, len);
}

simdutf_warn_unused result implementation::validate_utf16be_with_errors(
    const char16_t *buf, size_t len) const noexcept {
  return scalar::utf16::validate_with_errors<endianness::BIG>(buf, len);
}

void implementation::to_well_formed_utf16le(const char16_t *input, size_t len,
                                            char16_t *output) const noexcept {
  return scalar::utf16::to_well_formed_utf16<endianness::LITTLE>(input, len,
                                                                 output);
}

void implementation::to_well_formed_utf16be(const char16_t *input, size_t len,
                                            char16_t *output) const noexcept {
  return scalar::utf16::to_well_formed_utf16<endianness::BIG>(input, len,
                                                              output);
}
#endif // SIMDUTF_FEATURE_UTF16

#if SIMDUTF_FEATURE_UTF32 || SIMDUTF_FEATURE_DETECT_ENCODING
simdutf_warn_unused bool
implementation::validate_utf32(const char32_t *buf, size_t len) const noexcept {
  return scalar::utf32::validate(buf, len);
}
#endif // SIMDUTF_FEATURE_UTF32 || SIMDUTF_FEATURE_DETECT_ENCODING

#if SIMDUTF_FEATURE_UTF32
simdutf_warn_unused result implementation::validate_utf32_with_errors(
    const char32_t *buf, size_t len) const noexcept {
  return scalar::utf32::validate_with_errors(buf, len);
}
#endif // SIMDUTF_FEATURE_UTF32

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1
simdutf_warn_unused size_t implementation::convert_latin1_to_utf8(
    const char *buf, size_t len, char *utf8_output) const noexcept {
  return scalar::latin1_to_utf8::convert(buf, len, utf8_output);
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1

#if SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_LATIN1
simdutf_warn_unused size_t implementation::convert_latin1_to_utf16le(
    const char *buf, size_t len, char16_t *utf16_output) const noexcept {
  return scalar::latin1_to_utf16::convert<endianness::LITTLE>(buf, len,
                                                              utf16_output);
}

simdutf_warn_unused size_t implementation::convert_latin1_to_utf16be(
    const char *buf, size_t len, char16_t *utf16_output) const noexcept {
  return scalar::latin1_to_utf16::convert<endianness::BIG>(buf, len,
                                                           utf16_output);
}
#endif // SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_LATIN1

#if SIMDUTF_FEATURE_UTF32 && SIMDUTF_FEATURE_LATIN1
simdutf_warn_unused size_t implementation::convert_latin1_to_utf32(
    const char *buf, size_t len, char32_t *utf32_output) const noexcept {
  return scalar::latin1_to_utf32::convert(buf, len, utf32_output);
}
#endif // SIMDUTF_FEATURE_UTF32 && SIMDUTF_FEATURE_LATIN1

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1
simdutf_warn_unused size_t implementation::convert_utf8_to_latin1(
    const char *buf, size_t len, char *latin1_output) const noexcept {
  return scalar::utf8_to_latin1::convert(buf, len, latin1_output);
}

simdutf_warn_unused result implementation::convert_utf8_to_latin1_with_errors(
    const char *buf, size_t len, char *latin1_output) const noexcept {
  return scalar::utf8_to_latin1::convert_with_errors(buf, len, latin1_output);
}

simdutf_warn_unused size_t implementation::convert_valid_utf8_to_latin1(
    const char *buf, size_t len, char *latin1_output) const noexcept {
  return scalar::utf8_to_latin1::convert_valid(buf, len, latin1_output);
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16
simdutf_warn_unused size_t implementation::convert_utf8_to_utf16le(
    const char *buf, size_t len, char16_t *utf16_output) const noexcept {
  return scalar::utf8_to_utf16::convert<endianness::LITTLE>(buf, len,
                                                            utf16_output);
}

simdutf_warn_unused size_t implementation::convert_utf8_to_utf16be(
    const char *buf, size_t len, char16_t *utf16_output) const noexcept {
  return scalar::utf8_to_utf16::convert<endianness::BIG>(buf, len,
                                                         utf16_output);
}

simdutf_warn_unused result implementation::convert_utf8_to_utf16le_with_errors(
    const char *buf, size_t len, char16_t *utf16_output) const noexcept {
  return scalar::utf8_to_utf16::convert_with_errors<endianness::LITTLE>(
      buf, len, utf16_output);
}

simdutf_warn_unused result implementation::convert_utf8_to_utf16be_with_errors(
    const char *buf, size_t len, char16_t *utf16_output) const noexcept {
  return scalar::utf8_to_utf16::convert_with_errors<endianness::BIG>(
      buf, len, utf16_output);
}

simdutf_warn_unused size_t implementation::convert_valid_utf8_to_utf16le(
    const char *buf, size_t len, char16_t *utf16_output) const noexcept {
  return scalar::utf8_to_utf16::convert_valid<endianness::LITTLE>(buf, len,
                                                                  utf16_output);
}

simdutf_warn_unused size_t implementation::convert_valid_utf8_to_utf16be(
    const char *buf, size_t len, char16_t *utf16_output) const noexcept {
  return scalar::utf8_to_utf16::convert_valid<endianness::BIG>(buf, len,
                                                               utf16_output);
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32
simdutf_warn_unused size_t implementation::convert_utf8_to_utf32(
    const char *buf, size_t len, char32_t *utf32_output) const noexcept {
  return scalar::utf8_to_utf32::convert(buf, len, utf32_output);
}

simdutf_warn_unused result implementation::convert_utf8_to_utf32_with_errors(
    const char *buf, size_t len, char32_t *utf32_output) const noexcept {
  return scalar::utf8_to_utf32::convert_with_errors(buf, len, utf32_output);
}

simdutf_warn_unused size_t implementation::convert_valid_utf8_to_utf32(
    const char *input, size_t size, char32_t *utf32_output) const noexcept {
  return scalar::utf8_to_utf32::convert_valid(input, size, utf32_output);
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32

#if SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_LATIN1
simdutf_warn_unused size_t implementation::convert_utf16le_to_latin1(
    const char16_t *buf, size_t len, char *latin1_output) const noexcept {
  return scalar::utf16_to_latin1::convert<endianness::LITTLE>(buf, len,
                                                              latin1_output);
}

simdutf_warn_unused size_t implementation::convert_utf16be_to_latin1(
    const char16_t *buf, size_t len, char *latin1_output) const noexcept {
  return scalar::utf16_to_latin1::convert<endianness::BIG>(buf, len,
                                                           latin1_output);
}

simdutf_warn_unused result
implementation::convert_utf16le_to_latin1_with_errors(
    const char16_t *buf, size_t len, char *latin1_output) const noexcept {
  return scalar::utf16_to_latin1::convert_with_errors<endianness::LITTLE>(
      buf, len, latin1_output);
}

simdutf_warn_unused result
implementation::convert_utf16be_to_latin1_with_errors(
    const char16_t *buf, size_t len, char *latin1_output) const noexcept {
  return scalar::utf16_to_latin1::convert_with_errors<endianness::BIG>(
      buf, len, latin1_output);
}

simdutf_warn_unused size_t implementation::convert_valid_utf16le_to_latin1(
    const char16_t *buf, size_t len, char *latin1_output) const noexcept {
  return scalar::utf16_to_latin1::convert_valid<endianness::LITTLE>(
      buf, len, latin1_output);
}

simdutf_warn_unused size_t implementation::convert_valid_utf16be_to_latin1(
    const char16_t *buf, size_t len, char *latin1_output) const noexcept {
  return scalar::utf16_to_latin1::convert_valid<endianness::BIG>(buf, len,
                                                                 latin1_output);
}
#endif // SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_LATIN1

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16
simdutf_warn_unused size_t implementation::convert_utf16le_to_utf8(
    const char16_t *buf, size_t len, char *utf8_output) const noexcept {
  return scalar::utf16_to_utf8::convert<endianness::LITTLE>(buf, len,
                                                            utf8_output);
}

simdutf_warn_unused size_t implementation::convert_utf16be_to_utf8(
    const char16_t *buf, size_t len, char *utf8_output) const noexcept {
  return scalar::utf16_to_utf8::convert<endianness::BIG>(buf, len, utf8_output);
}

simdutf_warn_unused result implementation::convert_utf16le_to_utf8_with_errors(
    const char16_t *buf, size_t len, char *utf8_output) const noexcept {
  return scalar::utf16_to_utf8::convert_with_errors<endianness::LITTLE>(
      buf, len, utf8_output);
}

simdutf_warn_unused result implementation::convert_utf16be_to_utf8_with_errors(
    const char16_t *buf, size_t len, char *utf8_output) const noexcept {
  return scalar::utf16_to_utf8::convert_with_errors<endianness::BIG>(
      buf, len, utf8_output);
}

simdutf_warn_unused size_t implementation::convert_valid_utf16le_to_utf8(
    const char16_t *buf, size_t len, char *utf8_output) const noexcept {
  return scalar::utf16_to_utf8::convert_valid<endianness::LITTLE>(buf, len,
                                                                  utf8_output);
}

simdutf_warn_unused size_t implementation::convert_valid_utf16be_to_utf8(
    const char16_t *buf, size_t len, char *utf8_output) const noexcept {
  return scalar::utf16_to_utf8::convert_valid<endianness::BIG>(buf, len,
                                                               utf8_output);
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16

#if SIMDUTF_FEATURE_UTF32 && SIMDUTF_FEATURE_LATIN1
simdutf_warn_unused size_t implementation::convert_utf32_to_latin1(
    const char32_t *buf, size_t len, char *latin1_output) const noexcept {
  return scalar::utf32_to_latin1::convert(buf, len, latin1_output);
}

simdutf_warn_unused result implementation::convert_utf32_to_latin1_with_errors(
    const char32_t *buf, size_t len, char *latin1_output) const noexcept {
  return scalar::utf32_to_latin1::convert_with_errors(buf, len, latin1_output);
}

simdutf_warn_unused size_t implementation::convert_valid_utf32_to_latin1(
    const char32_t *buf, size_t len, char *latin1_output) const noexcept {
  return scalar::utf32_to_latin1::convert_valid(buf, len, latin1_output);
}
#endif // SIMDUTF_FEATURE_UTF32 && SIMDUTF_FEATURE_LATIN1

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32
simdutf_warn_unused size_t implementation::convert_utf32_to_utf8(
    const char32_t *buf, size_t len, char *utf8_output) const noexcept {
  return scalar::utf32_to_utf8::convert(buf, len, utf8_output);
}

simdutf_warn_unused result implementation::convert_utf32_to_utf8_with_errors(
    const char32_t *buf, size_t len, char *utf8_output) const noexcept {
  const utf32_ascii_progress ascii =
      copy_utf32_ascii_prefix(buf, len, utf8_output);
  #if defined(SIMDUTF_IS_X86_64) && !defined(SIMDUTF_REGULAR_VISUAL_STUDIO)
  if (fallback_sse41_supported()) {
    const utf32_packed_progress progress = sse41_pack_utf32_to_utf8(
        buf + ascii.input, len - ascii.input, utf8_output + ascii.output);
    const result tail = scalar::utf32_to_utf8::convert_with_errors(
        buf + ascii.input + progress.input, len - ascii.input - progress.input,
        utf8_output + ascii.output + progress.output);
    if (tail.error) {
      return result(tail.error, tail.count + ascii.input + progress.input);
    }
    return result(error_code::SUCCESS,
                  ascii.output + progress.output + tail.count);
  }
  #endif
  const result tail = scalar::utf32_to_utf8::convert_with_errors(
      buf + ascii.input, len - ascii.input, utf8_output + ascii.output);
  if (tail.error) {
    return result(tail.error, tail.count + ascii.input);
  }
  return result(error_code::SUCCESS, ascii.output + tail.count);
}

simdutf_warn_unused size_t implementation::convert_valid_utf32_to_utf8(
    const char32_t *buf, size_t len, char *utf8_output) const noexcept {
  return scalar::utf32_to_utf8::convert_valid(buf, len, utf8_output);
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32

#if SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_UTF32
simdutf_warn_unused size_t implementation::convert_utf32_to_utf16le(
    const char32_t *buf, size_t len, char16_t *utf16_output) const noexcept {
  return scalar::utf32_to_utf16::convert<endianness::LITTLE>(buf, len,
                                                             utf16_output);
}

simdutf_warn_unused size_t implementation::convert_utf32_to_utf16be(
    const char32_t *buf, size_t len, char16_t *utf16_output) const noexcept {
  return scalar::utf32_to_utf16::convert<endianness::BIG>(buf, len,
                                                          utf16_output);
}

simdutf_warn_unused result implementation::convert_utf32_to_utf16le_with_errors(
    const char32_t *buf, size_t len, char16_t *utf16_output) const noexcept {
  return scalar::utf32_to_utf16::convert_with_errors<endianness::LITTLE>(
      buf, len, utf16_output);
}

simdutf_warn_unused result implementation::convert_utf32_to_utf16be_with_errors(
    const char32_t *buf, size_t len, char16_t *utf16_output) const noexcept {
  return scalar::utf32_to_utf16::convert_with_errors<endianness::BIG>(
      buf, len, utf16_output);
}

simdutf_warn_unused size_t implementation::convert_valid_utf32_to_utf16le(
    const char32_t *buf, size_t len, char16_t *utf16_output) const noexcept {
  return scalar::utf32_to_utf16::convert_valid<endianness::LITTLE>(
      buf, len, utf16_output);
}

simdutf_warn_unused size_t implementation::convert_valid_utf32_to_utf16be(
    const char32_t *buf, size_t len, char16_t *utf16_output) const noexcept {
  return scalar::utf32_to_utf16::convert_valid<endianness::BIG>(buf, len,
                                                                utf16_output);
}

simdutf_warn_unused size_t implementation::convert_utf16le_to_utf32(
    const char16_t *buf, size_t len, char32_t *utf32_output) const noexcept {
  return scalar::utf16_to_utf32::convert<endianness::LITTLE>(buf, len,
                                                             utf32_output);
}

simdutf_warn_unused size_t implementation::convert_utf16be_to_utf32(
    const char16_t *buf, size_t len, char32_t *utf32_output) const noexcept {
  return scalar::utf16_to_utf32::convert<endianness::BIG>(buf, len,
                                                          utf32_output);
}

simdutf_warn_unused result implementation::convert_utf16le_to_utf32_with_errors(
    const char16_t *buf, size_t len, char32_t *utf32_output) const noexcept {
  return scalar::utf16_to_utf32::convert_with_errors<endianness::LITTLE>(
      buf, len, utf32_output);
}

simdutf_warn_unused result implementation::convert_utf16be_to_utf32_with_errors(
    const char16_t *buf, size_t len, char32_t *utf32_output) const noexcept {
  return scalar::utf16_to_utf32::convert_with_errors<endianness::BIG>(
      buf, len, utf32_output);
}

simdutf_warn_unused size_t implementation::convert_valid_utf16le_to_utf32(
    const char16_t *buf, size_t len, char32_t *utf32_output) const noexcept {
  return scalar::utf16_to_utf32::convert_valid<endianness::LITTLE>(
      buf, len, utf32_output);
}

simdutf_warn_unused size_t implementation::convert_valid_utf16be_to_utf32(
    const char16_t *buf, size_t len, char32_t *utf32_output) const noexcept {
  return scalar::utf16_to_utf32::convert_valid<endianness::BIG>(buf, len,
                                                                utf32_output);
}
#endif // SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_UTF32

#if SIMDUTF_FEATURE_UTF16
void implementation::change_endianness_utf16(const char16_t *input,
                                             size_t length,
                                             char16_t *output) const noexcept {
  scalar::utf16::change_endianness_utf16(input, length, output);
}

simdutf_warn_unused size_t implementation::count_utf16le(
    const char16_t *input, size_t length) const noexcept {
  return scalar::utf16::count_code_points<endianness::LITTLE>(input, length);
}

simdutf_warn_unused size_t implementation::count_utf16be(
    const char16_t *input, size_t length) const noexcept {
  return scalar::utf16::count_code_points<endianness::BIG>(input, length);
}
#endif // SIMDUTF_FEATURE_UTF16

#if SIMDUTF_FEATURE_UTF8
simdutf_warn_unused size_t
implementation::count_utf8(const char *input, size_t length) const noexcept {
  return scalar::utf8::count_code_points(input, length);
}
#endif // SIMDUTF_FEATURE_UTF8

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1
simdutf_warn_unused size_t implementation::latin1_length_from_utf8(
    const char *buf, size_t len) const noexcept {
  return scalar::utf8::count_code_points(buf, len);
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1
simdutf_warn_unused size_t implementation::utf8_length_from_latin1(
    const char *input, size_t length) const noexcept {
  return scalar::latin1_to_utf8::utf8_length_from_latin1(input, length);
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16
simdutf_warn_unused size_t implementation::utf8_length_from_utf16le(
    const char16_t *input, size_t length) const noexcept {
  return scalar::utf16::utf8_length_from_utf16<endianness::LITTLE>(input,
                                                                   length);
}

simdutf_warn_unused size_t implementation::utf8_length_from_utf16be(
    const char16_t *input, size_t length) const noexcept {
  return scalar::utf16::utf8_length_from_utf16<endianness::BIG>(input, length);
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16

#if SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_UTF32
simdutf_warn_unused size_t implementation::utf32_length_from_utf16le(
    const char16_t *input, size_t length) const noexcept {
  return scalar::utf16::utf32_length_from_utf16<endianness::LITTLE>(input,
                                                                    length);
}

simdutf_warn_unused size_t implementation::utf32_length_from_utf16be(
    const char16_t *input, size_t length) const noexcept {
  return scalar::utf16::utf32_length_from_utf16<endianness::BIG>(input, length);
}
#endif // SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_UTF32

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16
simdutf_warn_unused size_t implementation::utf16_length_from_utf8(
    const char *input, size_t length) const noexcept {
  return scalar::utf8::utf16_length_from_utf8(input, length);
}
simdutf_warn_unused result
implementation::utf8_length_from_utf16le_with_replacement(
    const char16_t *input, size_t length) const noexcept {
  return scalar::utf16::utf8_length_from_utf16_with_replacement<
      endianness::LITTLE>(input, length);
}

simdutf_warn_unused result
implementation::utf8_length_from_utf16be_with_replacement(
    const char16_t *input, size_t length) const noexcept {
  return scalar::utf16::utf8_length_from_utf16_with_replacement<
      endianness::BIG>(input, length);
}

simdutf_warn_unused size_t
implementation::convert_utf16le_to_utf8_with_replacement(
    const char16_t *input, size_t length, char *utf8_buffer) const noexcept {
  return scalar::utf16_to_utf8::convert_with_replacement<endianness::LITTLE>(
      input, length, utf8_buffer);
}

simdutf_warn_unused size_t
implementation::convert_utf16be_to_utf8_with_replacement(
    const char16_t *input, size_t length, char *utf8_buffer) const noexcept {
  return scalar::utf16_to_utf8::convert_with_replacement<endianness::BIG>(
      input, length, utf8_buffer);
}

#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32
simdutf_warn_unused size_t implementation::utf8_length_from_utf32(
    const char32_t *input, size_t length) const noexcept {
  return scalar::utf32::utf8_length_from_utf32(input, length);
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32

#if SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_UTF32
simdutf_warn_unused size_t implementation::utf16_length_from_utf32(
    const char32_t *input, size_t length) const noexcept {
  return scalar::utf32::utf16_length_from_utf32(input, length);
}
#endif // SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_UTF32

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32
simdutf_warn_unused size_t implementation::utf32_length_from_utf8(
    const char *input, size_t length) const noexcept {
  return scalar::utf8::count_code_points(input, length);
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32

#if SIMDUTF_FEATURE_BASE64

simdutf_warn_unused result implementation::base64_to_binary(
    const char *input, size_t length, char *output, base64_options options,
    last_chunk_handling_options last_chunk_options) const noexcept {
  return simdutf::scalar::base64::base64_to_binary_details_impl(
      input, length, output, options, last_chunk_options);
}

simdutf_warn_unused result implementation::base64_to_binary(
    const char16_t *input, size_t length, char *output, base64_options options,
    last_chunk_handling_options last_chunk_options) const noexcept {
  return simdutf::scalar::base64::base64_to_binary_details_impl(
      input, length, output, options, last_chunk_options);
}

simdutf_warn_unused full_result implementation::base64_to_binary_details(
    const char *input, size_t length, char *output, base64_options options,
    last_chunk_handling_options last_chunk_options) const noexcept {
  return simdutf::scalar::base64::base64_to_binary_details_impl(
      input, length, output, options, last_chunk_options);
}

simdutf_warn_unused full_result implementation::base64_to_binary_details(
    const char16_t *input, size_t length, char *output, base64_options options,
    last_chunk_handling_options last_chunk_options) const noexcept {
  return simdutf::scalar::base64::base64_to_binary_details_impl(
      input, length, output, options, last_chunk_options);
}

size_t implementation::binary_to_base64(const char *input, size_t length,
                                        char *output,
                                        base64_options options) const noexcept {
  return scalar::base64::tail_encode_base64(output, input, length, options);
}

size_t implementation::binary_to_base64_with_lines(
    const char *input, size_t length, char *output, size_t line_length,
    base64_options options) const noexcept {
  return scalar::base64::tail_encode_base64_impl<true>(output, input, length,
                                                       options, line_length);
}

const char *implementation::find(const char *start, const char *end,
                                 char character) const noexcept {
  for (; start < end; ++start) {
    if (*start == character) {
      return start;
    }
  }
  return end;
}

const char16_t *implementation::find(const char16_t *start, const char16_t *end,
                                     char16_t character) const noexcept {
  for (; start < end; ++start) {
    if (*start == character) {
      return start;
    }
  }
  return end;
}
#endif // SIMDUTF_FEATURE_BASE64

} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf

#include "simdutf/fallback/end.h"
