namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {

template <typename T> simdutf_really_inline T load_bytes_as(const uint8_t *p) {
  T v;
  std::memcpy(&v, p, sizeof(T));
  return v;
}

// Returns true if the n bytes at p are all ASCII, reading only those bytes.
// The validators use it on the last, partial block instead of copying that
// block to the stack, which would stall the vector load of the copy.
simdutf_really_inline bool tail_is_ascii(const uint8_t *p, size_t n) {
  uint64_t acc = 0;
  if (n >= 16) {
    for (size_t i = 0; i + 16 < n; i += 16) {
      acc |=
          load_bytes_as<uint64_t>(p + i) | load_bytes_as<uint64_t>(p + i + 8);
    }
    acc |= load_bytes_as<uint64_t>(p + n - 16) |
           load_bytes_as<uint64_t>(p + n - 8);
  } else if (n >= 8) {
    acc = load_bytes_as<uint64_t>(p) | load_bytes_as<uint64_t>(p + n - 8);
  } else if (n >= 4) {
    acc = load_bytes_as<uint32_t>(p) | load_bytes_as<uint32_t>(p + n - 4);
  } else {
    for (size_t i = 0; i < n; i++) {
      acc |= p[i];
    }
  }
  return (acc & 0x8080808080808080) == 0;
}

// Like tail_is_ascii(buf + idx, len - idx), for the remainder that
// buf_block_reader leaves at idx (1 to 64 bytes, unless len is 0). The caller
// must already know that the bytes before idx are ASCII. Inputs of at least
// one block are checked with one load of their last block instead, which
// overlaps bytes that were already checked: a non-ASCII byte there can only be
// at or after idx.
simdutf_really_inline bool remainder_is_ascii(const uint8_t *buf, size_t len,
                                              size_t idx) {
  constexpr size_t block = sizeof(simd::simd8x64<uint8_t>);
  if (len >= block) {
    return simd::simd8x64<uint8_t>(buf + len - block).is_ascii();
  }
  return tail_is_ascii(buf + idx, len - idx);
}

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
