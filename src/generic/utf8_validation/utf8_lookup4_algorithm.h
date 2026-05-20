namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {
namespace utf8_validation {

using namespace simd;

simdutf_really_inline simd8<uint8_t>
check_special_cases(const simd8<uint8_t> input, const simd8<uint8_t> prev1) {
  // Bit 0 = Too Short (lead byte/ASCII followed by lead byte/ASCII)
  // Bit 1 = Too Long (ASCII followed by continuation)
  // Bit 2 = Overlong 3-byte
  // Bit 4 = Surrogate
  // Bit 5 = Overlong 2-byte
  // Bit 7 = Two Continuations
  constexpr const uint8_t TOO_SHORT = 1 << 0;  // 11______ 0_______
                                               // 11______ 11______
  constexpr const uint8_t TOO_LONG = 1 << 1;   // 0_______ 10______
  constexpr const uint8_t OVERLONG_3 = 1 << 2; // 11100000 100_____
  constexpr const uint8_t SURROGATE = 1 << 4;  // 11101101 101_____
  constexpr const uint8_t OVERLONG_2 = 1 << 5; // 1100000_ 10______
  constexpr const uint8_t TWO_CONTS = 1 << 7;  // 10______ 10______
  constexpr const uint8_t TOO_LARGE = 1 << 3;  // 11110100 1001____
                                               // 11110100 101_____
                                               // 11110101 1001____
                                               // 11110101 101_____
                                               // 1111011_ 1001____
                                               // 1111011_ 101_____
                                               // 11111___ 1001____
                                               // 11111___ 101_____
  constexpr const uint8_t TOO_LARGE_1000 = 1 << 6;
  // 11110101 1000____
  // 1111011_ 1000____
  // 11111___ 1000____
  constexpr const uint8_t OVERLONG_4 = 1 << 6; // 11110000 1000____

  const simd8<uint8_t> byte_1_high = prev1.shr<4>().lookup_16<uint8_t>(
      // 0_______ ________ <ASCII in byte 1>
      TOO_LONG, TOO_LONG, TOO_LONG, TOO_LONG, TOO_LONG, TOO_LONG, TOO_LONG,
      TOO_LONG,
      // 10______ ________ <continuation in byte 1>
      TWO_CONTS, TWO_CONTS, TWO_CONTS, TWO_CONTS,
      // 1100____ ________ <two byte lead in byte 1>
      TOO_SHORT | OVERLONG_2,
      // 1101____ ________ <two byte lead in byte 1>
      TOO_SHORT,
      // 1110____ ________ <three byte lead in byte 1>
      TOO_SHORT | OVERLONG_3 | SURROGATE,
      // 1111____ ________ <four+ byte lead in byte 1>
      TOO_SHORT | TOO_LARGE | TOO_LARGE_1000 | OVERLONG_4);
  constexpr const uint8_t CARRY =
      TOO_SHORT | TOO_LONG | TWO_CONTS; // These all have ____ in byte 1 .
  const simd8<uint8_t> byte_1_low =
      (prev1 & 0x0F)
          .lookup_16<uint8_t>(
              // ____0000 ________
              CARRY | OVERLONG_3 | OVERLONG_2 | OVERLONG_4,
              // ____0001 ________
              CARRY | OVERLONG_2,
              // ____001_ ________
              CARRY, CARRY,

              // ____0100 ________
              CARRY | TOO_LARGE,
              // ____0101 ________
              CARRY | TOO_LARGE | TOO_LARGE_1000,
              // ____011_ ________
              CARRY | TOO_LARGE | TOO_LARGE_1000,
              CARRY | TOO_LARGE | TOO_LARGE_1000,

              // ____1___ ________
              CARRY | TOO_LARGE | TOO_LARGE_1000,
              CARRY | TOO_LARGE | TOO_LARGE_1000,
              CARRY | TOO_LARGE | TOO_LARGE_1000,
              CARRY | TOO_LARGE | TOO_LARGE_1000,
              CARRY | TOO_LARGE | TOO_LARGE_1000,
              // ____1101 ________
              CARRY | TOO_LARGE | TOO_LARGE_1000 | SURROGATE,
              CARRY | TOO_LARGE | TOO_LARGE_1000,
              CARRY | TOO_LARGE | TOO_LARGE_1000);
  const simd8<uint8_t> byte_2_high = input.shr<4>().lookup_16<uint8_t>(
      // ________ 0_______ <ASCII in byte 2>
      TOO_SHORT, TOO_SHORT, TOO_SHORT, TOO_SHORT, TOO_SHORT, TOO_SHORT,
      TOO_SHORT, TOO_SHORT,

      // ________ 1000____
      TOO_LONG | OVERLONG_2 | TWO_CONTS | OVERLONG_3 | TOO_LARGE_1000 |
          OVERLONG_4,
      // ________ 1001____
      TOO_LONG | OVERLONG_2 | TWO_CONTS | OVERLONG_3 | TOO_LARGE,
      // ________ 101_____
      TOO_LONG | OVERLONG_2 | TWO_CONTS | SURROGATE | TOO_LARGE,
      TOO_LONG | OVERLONG_2 | TWO_CONTS | SURROGATE | TOO_LARGE,

      // ________ 11______
      TOO_SHORT, TOO_SHORT, TOO_SHORT, TOO_SHORT);
  return (byte_1_high & byte_1_low & byte_2_high);
}
simdutf_really_inline simd8<uint8_t>
check_multibyte_lengths(const simd8<uint8_t> input,
                        const simd8<uint8_t> prev_input,
                        const simd8<uint8_t> sc) {
  simd8<uint8_t> prev2 = input.prev<2>(prev_input);
  simd8<uint8_t> prev3 = input.prev<3>(prev_input);
  simd8<uint8_t> must23 =
      simd8<uint8_t>(must_be_2_3_continuation(prev2, prev3));
  simd8<uint8_t> must23_80 = must23 & uint8_t(0x80);
  return must23_80 ^ sc;
}

//
// Return nonzero if there are incomplete multibyte characters at the end of the
// block: e.g. if there is a 4-byte character, but it is 3 bytes from the end.
//
simdutf_really_inline simd8<uint8_t> is_incomplete(const simd8<uint8_t> input) {
  // If the previous input's last 3 bytes match this, they're too short (they
  // ended at EOF):
  // ... 1111____ 111_____ 11______
  static const uint8_t max_array[32] = {255,
                                        255,
                                        255,
                                        255,
                                        255,
                                        255,
                                        255,
                                        255,
                                        255,
                                        255,
                                        255,
                                        255,
                                        255,
                                        255,
                                        255,
                                        255,
                                        255,
                                        255,
                                        255,
                                        255,
                                        255,
                                        255,
                                        255,
                                        255,
                                        255,
                                        255,
                                        255,
                                        255,
                                        255,
                                        0b11110000u - 1,
                                        0b11100000u - 1,
                                        0b11000000u - 1};
  const simd8<uint8_t> max_value(
      &max_array[sizeof(max_array) - sizeof(simd8<uint8_t>)]);
  return input.gt_bits(max_value);
}

simdutf_really_inline std::pair<size_t, size_t>
count_cont_4byte(const simd8<uint8_t> input) {
  // The SimdUnicode implementation uses a SIMD function to extract the most
  // significant bits. In the generic code we don't have that functionality, so
  // we make due with more comparison.
  uint64_t utf8_continuation_mask = input.lt(-65 + 1);
  size_t continuations = count_ones(utf8_continuation_mask);
  int64_t utf8_4byte = input.gteq_unsigned(0b11110000);
  size_t four_byte = count_ones(utf8_4byte);
  return std::make_pair(continuations, four_byte);
}

struct utf8_checker {
  // If this is nonzero, there has been a UTF-8 error.
  simd8<uint8_t> error;
  // The last input we received
  simd8<uint8_t> prev_input_block;
  // Whether the last input we received was incomplete (used for ASCII fast
  // path)
  simd8<uint8_t> prev_incomplete;

  //
  // Check whether the current bytes are valid UTF-8.
  //
  simdutf_really_inline void check_utf8_bytes(const simd8<uint8_t> input,
                                              const simd8<uint8_t> prev_input) {
    // Flip prev1...prev3 so we can easily determine if they are 2+, 3+ or 4+
    // lead bytes (2, 3, 4-byte leads become large positive numbers instead of
    // small negative numbers)
    simd8<uint8_t> prev1 = input.prev<1>(prev_input);
    simd8<uint8_t> sc = check_special_cases(input, prev1);
    this->error |= check_multibyte_lengths(input, prev_input, sc);
  }

  // The only problem that can happen at EOF is that a multibyte character is
  // too short or a byte value too large in the last bytes: check_special_cases
  // only checks for bytes too large in the first of two bytes.
  simdutf_really_inline void check_eof() {
    // If the previous block had incomplete UTF-8 characters at the end, an
    // ASCII block can't possibly finish them.
    this->error |= this->prev_incomplete;
  }

  simdutf_really_inline void check_next_input(const simd8x64<uint8_t> &input) {
    if (simdutf_likely(is_ascii(input))) {
      this->error |= this->prev_incomplete;
    } else {
      // you might think that a for-loop would work, but under Visual Studio, it
      // is not good enough.
      static_assert((simd8x64<uint8_t>::NUM_CHUNKS == 2) ||
                        (simd8x64<uint8_t>::NUM_CHUNKS == 4),
                    "We support either two or four chunks per 64-byte block.");
      if (simd8x64<uint8_t>::NUM_CHUNKS == 2) {
        this->check_utf8_bytes(input.chunks[0], this->prev_input_block);
        this->check_utf8_bytes(input.chunks[1], input.chunks[0]);
      } else if (simd8x64<uint8_t>::NUM_CHUNKS == 4) {
        this->check_utf8_bytes(input.chunks[0], this->prev_input_block);
        this->check_utf8_bytes(input.chunks[1], input.chunks[0]);
        this->check_utf8_bytes(input.chunks[2], input.chunks[1]);
        this->check_utf8_bytes(input.chunks[3], input.chunks[2]);
      }
      this->prev_incomplete =
          is_incomplete(input.chunks[simd8x64<uint8_t>::NUM_CHUNKS - 1]);
      this->prev_input_block = input.chunks[simd8x64<uint8_t>::NUM_CHUNKS - 1];
    }
  }

  // do not forget to call check_eof!
  simdutf_really_inline bool errors() const {
    return this->error.any_bits_set_anywhere();
  }

}; // struct utf8_checker

struct utf8_segmenter {
  utf8_checker checker;
  // Counter for continuations, unused by pure validation.
  size_t continuations;
  // Counter for 4-byte leads, unused by pure validation.
  size_t four_byte;

  //
  // Check whether the current bytes are valid UTF-8 and update continuation and
  // 4-byte lead counts.
  //
  simdutf_really_inline void
  check_utf8_bytes_with_counts(const simd8<uint8_t> input,
                               const simd8<uint8_t> prev_input) {
    std::pair<size_t, size_t> cont_4byte = count_cont_4byte(input);
    this->continuations += cont_4byte.first;
    this->four_byte += cont_4byte.second;
    this->checker.check_utf8_bytes(input, prev_input)
  }

  simdutf_really_inline void check_eof() { this->checker.check_eof(); }

  simdutf_really_inline std::pair<size_t, size_t>
  check_next_input_with_counts(const simd8x64<uint8_t> &input) {
    if (simdutf_likely(is_ascii(input))) {
      this->error |= this->prev_incomplete;
      return std::make_pair(0, 0);
    } else {
      size_t prev_continuations = this->continuations;
      size_t prev_four_byte = this->four_byte;
      // you might think that a for-loop would work, but under Visual Studio, it
      // is not good enough.
      static_assert((simd8x64<uint8_t>::NUM_CHUNKS == 2) ||
                        (simd8x64<uint8_t>::NUM_CHUNKS == 4),
                    "We support either two or four chunks per 64-byte block.");
      if (simd8x64<uint8_t>::NUM_CHUNKS == 2) {
        this->check_utf8_bytes(input.chunks[0], this->prev_input_block);
        this->check_utf8_bytes(input.chunks[1], input.chunks[0]);
      } else if (simd8x64<uint8_t>::NUM_CHUNKS == 4) {
        this->check_utf8_bytes(input.chunks[0], this->prev_input_block);
        this->check_utf8_bytes(input.chunks[1], input.chunks[0]);
        this->check_utf8_bytes(input.chunks[2], input.chunks[1]);
        this->check_utf8_bytes(input.chunks[3], input.chunks[2]);
      }
      this->prev_incomplete =
          is_incomplete(input.chunks[simd8x64<uint8_t>::NUM_CHUNKS - 1]);
      this->prev_input_block = input.chunks[simd8x64<uint8_t>::NUM_CHUNKS - 1];
      return std::make_pair(this->continuations - prev_continuations,
                            this->four_byte - prev_4_byte)
    }
  }

  // do not forget to call check_eof!
  simdutf_really_inline bool errors() const { return this->checker.errors(); }

  simdutf_really_inline size_t continuation_count() const {
    return this->continuations;
  }

  simdutf_really_inline size_t four_byte_count() const {
    return this->four_byte;
  }

}; // struct utf8_segmenter

} // namespace utf8_validation

using utf8_validation::utf8_checker;
using utf8_validation::utf8_segmenter;

} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf
