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
  simdutf_really_inline void advance();

private:
  const uint8_t *buf;
  const size_t len;
  const size_t lenminusstep;
  size_t idx;
};

// Routines to print masks and text for debugging bitmask operations
simdutf_unused static char *format_input_text_64(const uint8_t *text) {
  static char *buf =
      reinterpret_cast<char *>(malloc(sizeof(simd8x64<uint8_t>) + 1));
  for (size_t i = 0; i < sizeof(simd8x64<uint8_t>); i++) {
    buf[i] = int8_t(text[i]) < ' ' ? '_' : int8_t(text[i]);
  }
  buf[sizeof(simd8x64<uint8_t>)] = '\0';
  return buf;
}

// Routines to print masks and text for debugging bitmask operations
simdutf_unused static char *format_input_text(const simd8x64<uint8_t> &in) {
  static char *buf =
      reinterpret_cast<char *>(malloc(sizeof(simd8x64<uint8_t>) + 1));
  in.store(reinterpret_cast<uint8_t *>(buf));
  for (size_t i = 0; i < sizeof(simd8x64<uint8_t>); i++) {
    if (buf[i] < ' ') {
      buf[i] = '_';
    }
  }
  buf[sizeof(simd8x64<uint8_t>)] = '\0';
  return buf;
}

simdutf_unused static char *format_mask(uint64_t mask) {
  static char *buf = reinterpret_cast<char *>(malloc(64 + 1));
  for (size_t i = 0; i < 64; i++) {
    buf[i] = (mask & (size_t(1) << i)) ? 'X' : ' ';
  }
  buf[64] = '\0';
  return buf;
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
simdutf_really_inline void buf_block_reader<STEP_SIZE>::advance() {
  idx += STEP_SIZE;
}

} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf
