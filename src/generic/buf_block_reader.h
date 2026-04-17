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
  std::memset(dst, 0x20,
              STEP_SIZE); // std::memset STEP_SIZE because it is more efficient
                          // to write out 8 or 16 bytes at once.
  std::memcpy(dst, buf + idx, len - idx);
  return len - idx;
}

template <size_t STEP_SIZE>
simdutf_really_inline void buf_block_reader<STEP_SIZE>::advance() {
  idx += STEP_SIZE;
}

} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf
