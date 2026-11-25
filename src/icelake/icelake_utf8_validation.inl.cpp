// file included directly

simdutf_really_inline __m512i check_special_cases(__m512i input,
                                                  const __m512i prev1) {
  __m512i mask1 = _mm512_setr_epi64(0x0202020202020202, 0x4915012180808080,
                                    0x0202020202020202, 0x4915012180808080,
                                    0x0202020202020202, 0x4915012180808080,
                                    0x0202020202020202, 0x4915012180808080);
  const __m512i v_0f = _mm512_set1_epi8(0x0f);
  __m512i index1 = _mm512_and_si512(_mm512_srli_epi16(prev1, 4), v_0f);

  __m512i byte_1_high = _mm512_shuffle_epi8(mask1, index1);
  __m512i mask2 = _mm512_setr_epi64(0xcbcbcb8b8383a3e7, 0xcbcbdbcbcbcbcbcb,
                                    0xcbcbcb8b8383a3e7, 0xcbcbdbcbcbcbcbcb,
                                    0xcbcbcb8b8383a3e7, 0xcbcbdbcbcbcbcbcb,
                                    0xcbcbcb8b8383a3e7, 0xcbcbdbcbcbcbcbcb);
  __m512i index2 = _mm512_and_si512(prev1, v_0f);

  __m512i byte_1_low = _mm512_shuffle_epi8(mask2, index2);
  __m512i mask3 =
      _mm512_setr_epi64(0x101010101010101, 0x1010101babaaee6, 0x101010101010101,
                        0x1010101babaaee6, 0x101010101010101, 0x1010101babaaee6,
                        0x101010101010101, 0x1010101babaaee6);
  __m512i index3 = _mm512_and_si512(_mm512_srli_epi16(input, 4), v_0f);
  __m512i byte_2_high = _mm512_shuffle_epi8(mask3, index3);
  return _mm512_ternarylogic_epi64(byte_1_high, byte_1_low, byte_2_high, 128);
}

simdutf_really_inline __m512i check_multibyte_lengths(const __m512i input,
                                                      const __m512i prev_input,
                                                      const __m512i sc) {
  __m512i prev2 = prev<2>(input, prev_input);
  __m512i prev3 = prev<3>(input, prev_input);
  __m512i is_third_byte = _mm512_subs_epu8(
      prev2, _mm512_set1_epi8(0b11100000u - 1)); // Only 111_____ will be > 0
  __m512i is_fourth_byte = _mm512_subs_epu8(
      prev3, _mm512_set1_epi8(0b11110000u - 1)); // Only 1111____ will be > 0
  __m512i is_third_or_fourth_byte =
      _mm512_or_si512(is_third_byte, is_fourth_byte);
  const __m512i v_7f = _mm512_set1_epi8(char(0x7f));
  is_third_or_fourth_byte = _mm512_adds_epu8(v_7f, is_third_or_fourth_byte);
  // We want to compute (is_third_or_fourth_byte AND v80) XOR sc.
  const __m512i v_80 = _mm512_set1_epi8(char(0x80));
  return _mm512_ternarylogic_epi32(is_third_or_fourth_byte, v_80, sc,
                                   0b1101010);
  //__m512i is_third_or_fourth_byte_mask =
  //_mm512_and_si512(is_third_or_fourth_byte, v_80); return
  // _mm512_xor_si512(is_third_or_fourth_byte_mask, sc);
}
//
// Return nonzero if there are incomplete multibyte characters at the end of the
// block: e.g. if there is a 4-byte character, but it is 3 bytes from the end.
//
simdutf_really_inline __m512i is_incomplete(const __m512i input) {
  // If the previous input's last 3 bytes match this, they're too short (they
  // ended at EOF):
  // ... 1111____ 111_____ 11______
  __m512i max_value = _mm512_setr_epi64(0xffffffffffffffff, 0xffffffffffffffff,
                                        0xffffffffffffffff, 0xffffffffffffffff,
                                        0xffffffffffffffff, 0xffffffffffffffff,
                                        0xffffffffffffffff, 0xbfdfefffffffffff);
  return _mm512_subs_epu8(input, max_value);
}

struct avx512_utf8_checker {
  // If this is nonzero, there has been a UTF-8 error.
  __m512i error{};

  // The last input we received
  __m512i prev_input_block{};
  // Whether the last input we received was incomplete (used for ASCII fast
  // path)
  __m512i prev_incomplete{};

  //
  // Check whether the current bytes are valid UTF-8.
  //
  simdutf_really_inline void check_utf8_bytes(const __m512i input,
                                              const __m512i prev_input) {
    // Flip prev1...prev3 so we can easily determine if they are 2+, 3+ or 4+
    // lead bytes (2, 3, 4-byte leads become large positive numbers instead of
    // small negative numbers)
    __m512i prev1 = prev<1>(input, prev_input);
    __m512i sc = check_special_cases(input, prev1);
    this->error = _mm512_or_si512(
        check_multibyte_lengths(input, prev_input, sc), this->error);
  }

  // The only problem that can happen at EOF is that a multibyte character is
  // too short or a byte value too large in the last bytes: check_special_cases
  // only checks for bytes too large in the first of two bytes.
  simdutf_really_inline void check_eof() {
    // If the previous block had incomplete UTF-8 characters at the end, an
    // ASCII block can't possibly finish them.
    this->error = _mm512_or_si512(this->error, this->prev_incomplete);
  }

  // returns true if ASCII.
  simdutf_really_inline bool check_next_input(const __m512i input) {
    const __m512i v_80 = _mm512_set1_epi8(char(0x80));
    const __mmask64 ascii = _mm512_test_epi8_mask(input, v_80);
    if (ascii == 0) {
      this->error = _mm512_or_si512(this->error, this->prev_incomplete);
      return true;
    } else {
      this->check_utf8_bytes(input, this->prev_input_block);
      this->prev_incomplete = is_incomplete(input);
      this->prev_input_block = input;
      return false;
    }
  }
  // do not forget to call check_eof!
  simdutf_really_inline bool errors() const {
    return _mm512_test_epi8_mask(this->error, this->error) != 0;
  }
}; // struct avx512_utf8_checker

/*
 * Adapted from ffutf by Peter Fors (MIT licensed):
 * https://github.com/vtlmks/ffutf
 */
namespace utf8_fast_validation {

alignas(64) constexpr uint8_t second_bounds[64] = {
    0xc0, 0xc0, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0xa0,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x20, 0x80, 0x80, 0x90, 0x80, 0x80, 0x80, 0x10, 0xc0, 0xc0,
    0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0};

alignas(64) constexpr uint8_t next_index[64] = {
    1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
    33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
    49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 0};

simdutf_really_inline uint64_t check_span(__m512i bytes, __m512i next,
                                          uint64_t &carry) {
  const uint64_t lead =
      _mm512_cmpge_epu8_mask(bytes, _mm512_set1_epi8(char(0xc0)));
  const uint64_t three =
      _mm512_cmpge_epu8_mask(bytes, _mm512_set1_epi8(char(0xe0)));
  const uint64_t four =
      _mm512_cmpge_epu8_mask(bytes, _mm512_set1_epi8(char(0xf0)));
  const uint64_t cont =
      _mm512_cmplt_epi8_mask(bytes, _mm512_set1_epi8(char(0xc0)));
  const __m512i bounds = _mm512_permutexvar_epi8(
      bytes, _mm512_load_si512((const __m512i *)second_bounds));
  const __m512i delta = _mm512_maskz_sub_epi8(lead, next, bounds);
  const uint64_t bad =
      (cont ^ ((lead << 1) | (three << 2) | (four << 3) | carry)) |
      _mm512_movepi8_mask(delta);
  carry = (lead >> 63) | (three >> 62) | (four >> 61);
  return bad;
}

simdutf_really_inline uint64_t check_block(__m512i bytes, uint64_t carry,
                                           const uint8_t *input, size_t len) {
  const uint64_t nonascii = _mm512_movepi8_mask(bytes);
  if (!nonascii) {
    return carry ? UINT64_MAX : 0;
  }
  const uint64_t lead =
      _mm512_cmpge_epu8_mask(bytes, _mm512_set1_epi8(char(0xc0)));
  const uint64_t three =
      _mm512_cmpge_epu8_mask(bytes, _mm512_set1_epi8(char(0xe0)));
  const uint64_t four =
      _mm512_cmpge_epu8_mask(bytes, _mm512_set1_epi8(char(0xf0)));
  if ((nonascii ^ lead) != ((lead << 1) | (three << 2) | (four << 3) | carry)) {
    return UINT64_MAX;
  }
  const __m512i bounds = _mm512_permutexvar_epi8(
      bytes, _mm512_load_si512((const __m512i *)second_bounds));
  if (_mm512_mask_cmpneq_epi8_mask(lead, bounds,
                                   _mm512_set1_epi8(char(0x80)))) {
    const uint64_t active =
        len > 64 ? UINT64_MAX : (UINT64_C(1) << (len - 1)) - 1;
    const __m512i next =
        _mm512_maskz_loadu_epi8(active, (const __m512i *)(input + 1));
    const __m512i delta = _mm512_maskz_sub_epi8(lead, next, bounds);
    if (_mm512_movepi8_mask(delta)) {
      return UINT64_MAX;
    }
  }
  return (lead >> 63) | (three >> 62) | (four >> 61);
}

simdutf_really_inline __m512i next_of_last(__m512i bytes) {
  return _mm512_permutexvar_epi8(_mm512_load_si512((const __m512i *)next_index),
                                 bytes);
}

simdutf_really_inline bool check_last(__m512i bytes, uint64_t carry) {
  return (check_span(bytes, next_of_last(bytes), carry) | carry) == 0;
}

simdutf_really_inline const uint8_t *find_error_block(const uint8_t *input,
                                                      size_t len) {
  const uint8_t *const start = input;
  const size_t offset = reinterpret_cast<uintptr_t>(input) & 63;
  if (len >= 256 && offset &&
      !_mm512_movepi8_mask(_mm512_loadu_si512((const __m512i *)input))) {
    input += 64 - offset;
    len -= 64 - offset;
  }
  for (; len >= 256; input += 256, len -= 256) {
    const __m512i first = _mm512_loadu_si512((const __m512i *)input);
    const __m512i second = _mm512_loadu_si512((const __m512i *)(input + 64));
    const __m512i third = _mm512_loadu_si512((const __m512i *)(input + 128));
    const __m512i fourth = _mm512_loadu_si512((const __m512i *)(input + 192));
    if (_mm512_movepi8_mask(_mm512_or_si512(
            _mm512_ternarylogic_epi32(first, second, third, 0xfe), fourth))) {
      break;
    }
  }
  if (len == 0) {
    return nullptr;
  }
  if (len < 64) {
    const __m512i bytes = _mm512_maskz_loadu_epi8((UINT64_C(1) << len) - 1,
                                                  (const __m512i *)input);
    if (!_mm512_movepi8_mask(bytes) || check_last(bytes, 0)) {
      return nullptr;
    }
    return input;
  }
  const __m512i head = _mm512_loadu_si512((const __m512i *)input);
  if (len < 256 && !_mm512_movepi8_mask(head)) {
    if (len == 64) {
      return nullptr;
    }
    __m512i tail = _mm512_loadu_si512((const __m512i *)(input + len - 64));
    if (len > 128) {
      tail = _mm512_or_si512(tail,
                             _mm512_loadu_si512((const __m512i *)(input + 64)));
    }
    if (len > 192) {
      tail = _mm512_or_si512(
          tail, _mm512_loadu_si512((const __m512i *)(input + 128)));
    }
    if (!_mm512_movepi8_mask(tail)) {
      return nullptr;
    }
  }
  uint64_t carry = 0;
  __m512i bytes = head;
  if (len <= 1024) {
    const uint8_t *region = input;
    uint64_t bad = 0;
    for (; len >= 128; input += 64, len -= 64) {
      bad |= check_span(bytes, _mm512_loadu_si512((const __m512i *)(input + 1)),
                        carry);
      bytes = _mm512_loadu_si512((const __m512i *)(input + 64));
    }
    if (len > 64) {
      bad |= check_span(bytes, _mm512_loadu_si512((const __m512i *)(input + 1)),
                        carry);
      bytes = _mm512_maskz_loadu_epi8(UINT64_MAX >> (128 - len),
                                      (const __m512i *)(input + 64));
    }
    bad |= check_span(bytes, next_of_last(bytes), carry);
    return (bad | carry) == 0 ? nullptr : region;
  }
  carry = check_block(head, 0, input, len);
  if (carry == UINT64_MAX) {
    return input;
  }
  size_t step = 64 - (reinterpret_cast<uintptr_t>(input) & 63);
  if (input + step < start + 3) {
    step = 64;
  }
  input += step;
  len -= step;
  const uint8_t *const end = input + len;
  for (; end - input >= 64; input += 64) {
    __m512i current = _mm512_loadu_si512((const __m512i *)input);
    if (!_mm512_movepi8_mask(current)) {
      if (input[-1] >= 0xc0 || input[-2] >= 0xe0 || input[-3] >= 0xf0) {
        return input - 1;
      }
      do {
        input += 64;
        if (end - input < 64) {
          goto tail;
        }
        current = _mm512_loadu_si512((const __m512i *)input);
      } while (!_mm512_movepi8_mask(current));
    }
    const __m512i previous = _mm512_loadu_si512((const __m512i *)(input - 1));
    const uint64_t cont =
        _mm512_cmplt_epi8_mask(current, _mm512_set1_epi8(char(0xc0)));
    const uint64_t first =
        _mm512_cmpge_epu8_mask(previous, _mm512_set1_epi8(char(0xc0)));
    const uint64_t second =
        _mm512_cmpge_epu8_mask(_mm512_loadu_si512((const __m512i *)(input - 2)),
                               _mm512_set1_epi8(char(0xe0)));
    const uint64_t third =
        _mm512_cmpge_epu8_mask(_mm512_loadu_si512((const __m512i *)(input - 3)),
                               _mm512_set1_epi8(char(0xf0)));
    const __m512i bounds = _mm512_permutexvar_epi8(
        previous, _mm512_load_si512((const __m512i *)second_bounds));
    const __m512i delta = _mm512_maskz_sub_epi8(first, current, bounds);
    if (((cont ^ (first | second | third)) | _mm512_movepi8_mask(delta)) != 0) {
      return input - 1;
    }
  }
tail:
  len = size_t(end - input);
  const uint8_t one = input[-1];
  const uint8_t two = input[-2];
  const uint8_t three = input[-3];
  if (len && one >= 0xc0 &&
      uint8_t(input[0] - second_bounds[one & 63]) >= 0x80) {
    return input - 1;
  }
  carry = uint64_t(one >= 0xc0) | (uint64_t(one >= 0xe0) << 1) |
          (uint64_t(one >= 0xf0) << 2) | uint64_t(two >= 0xe0) |
          (uint64_t(two >= 0xf0) << 1) | uint64_t(three >= 0xf0);
  if (len) {
    const __m512i last = _mm512_maskz_loadu_epi8(UINT64_MAX >> (64 - len),
                                                 (const __m512i *)input);
    carry = check_block(last, carry, input, len);
  }
  return carry == 0 ? nullptr : input - 1;
}

} // namespace utf8_fast_validation
