namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {

// Walks through a buffer in block-sized increments, loading the last part with
// spaces
template <size_t STEP_SIZE> struct buf_block_reader {
public:
  simdutf_really_inline buf_block_reader(const uint8_t *_buf, size_t _len);
  simdutf_really_inline size_t block_index();
  simdutf_really_inline bool has_full_block() const;
  simdutf_really_inline const uint8_t *full_block() const;
  /**
   * Get the last block, padded with spaces.
   *
   * There will always be a last block, with at least 1 byte, unless len == 0
   * (in which case this function fills the buffer with spaces and returns 0. In
   * particular, if len == STEP_SIZE there will be 0 full_blocks and 1 remainder
   * block with STEP_SIZE bytes and no spaces for padding.
   *
   * @return the number of effective characters in the last block.
   */
  simdutf_really_inline size_t get_remainder(uint8_t *dst) const;
  simdutf_really_inline void advance();

private:
  const uint8_t *buf;
  const size_t len;
  const size_t lenminusstep;
  size_t idx;
};

simdutf_unused static char
    format_input_text_64_buf[sizeof(simd8x64<uint8_t>) + 1];
simdutf_unused static char format_input_text_buf[sizeof(simd8x64<uint8_t>) + 1];
simdutf_unused static char format_mask_buf[64 + 1];

// Routines to print masks and text for debugging bitmask operations
simdutf_unused static char *format_input_text_64(const uint8_t *text) {
  for (size_t i = 0; i < sizeof(simd8x64<uint8_t>); i++) {
    format_input_text_64_buf[i] = int8_t(text[i]) < ' ' ? '_' : int8_t(text[i]);
  }
  format_input_text_64_buf[sizeof(simd8x64<uint8_t>)] = '\0';
  return format_input_text_64_buf;
}

// Routines to print masks and text for debugging bitmask operations
simdutf_unused static char *format_input_text(const simd8x64<uint8_t> &in) {
  in.store(reinterpret_cast<uint8_t *>(format_input_text_buf));
  for (size_t i = 0; i < sizeof(simd8x64<uint8_t>); i++) {
    if (format_input_text_buf[i] < ' ') {
      format_input_text_buf[i] = '_';
    }
  }
  format_input_text_buf[sizeof(simd8x64<uint8_t>)] = '\0';
  return format_input_text_buf;
}

simdutf_unused static char *format_mask(uint64_t mask) {
  for (size_t i = 0; i < 64; i++) {
    format_mask_buf[i] = (mask & (size_t(1) << i)) ? 'X' : ' ';
  }
  format_mask_buf[64] = '\0';
  return format_mask_buf;
}

template <size_t STEP_SIZE>
simdutf_really_inline
buf_block_reader<STEP_SIZE>::buf_block_reader(const uint8_t *_buf, size_t _len)
    : buf{_buf}, len{_len}, lenminusstep{len < STEP_SIZE ? 0 : len - STEP_SIZE},
      idx{0} {}

template <size_t STEP_SIZE>
simdutf_really_inline size_t buf_block_reader<STEP_SIZE>::block_index() {
  return idx;
}

template <size_t STEP_SIZE>
simdutf_really_inline bool buf_block_reader<STEP_SIZE>::has_full_block() const {
  return idx < lenminusstep;
}

template <size_t STEP_SIZE>
simdutf_really_inline const uint8_t *
buf_block_reader<STEP_SIZE>::full_block() const {
  return &buf[idx];
}

template <size_t STEP_SIZE>
simdutf_really_inline size_t
buf_block_reader<STEP_SIZE>::get_remainder(uint8_t *dst) const {
  if (len == idx) {
    return 0;
  } // memcpy(dst, null, 0) will trigger an error with some sanitizers
  simdutf::internal::memset(
      dst, 0x20,
      STEP_SIZE); // memset STEP_SIZE because it is more efficient to write out
                  // 8 or 16 bytes at once.
  simdutf::internal::memcpy(dst, buf + idx, len - idx);
  return len - idx;
}

template <size_t STEP_SIZE>
simdutf_really_inline void buf_block_reader<STEP_SIZE>::advance() {
  idx += STEP_SIZE;
}

} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf
