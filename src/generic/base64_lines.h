/**
 * Fast path for base64 split into lines of a fixed length (MIME, PEM).
 *
 * Inspired by ffbase64 by Peter Fors, a decoder specialized for MIME base64
 * with 76-character CRLF lines: https://github.com/vtlmks/ffbase64
 */
namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {
namespace base64_lines {

struct step {
  uint64_t blend;
  uint16_t expect;
  uint16_t mask;
  uint8_t gap;
  uint8_t advance;
  uint8_t check;
};

constexpr size_t max_cycle = 128;
constexpr size_t max_group = 4;

struct progress {
  size_t read;
  size_t written;
};

struct plan {
  size_t cycle;
  size_t eol;
  step steps[max_cycle + max_group - 1];
};

simdutf_really_inline uint16_t check_eol(const char *src, const step &s) {
  uint16_t word;
  std::memcpy(&word, src + s.check, sizeof(word));
  return uint16_t((word ^ s.expect) & s.mask);
}

simdutf_really_inline bool has_room(const char *src, const char *srcend,
                                    size_t offset = 0) {
  return size_t(srcend - src) >= offset + 68;
}

static inline bool make_plan(const char *src, size_t len, plan &p,
                             size_t min_len) {
  if (len < min_len) {
    return false;
  }
  const void *newline = std::memchr(src, '\n', len < 1024 ? len : 1024);
  if (newline == nullptr) {
    return false;
  }
  const size_t n = size_t(static_cast<const char *>(newline) - src);
  const size_t eol = (n > 0 && src[n - 1] == '\r') ? 2 : 1;
  const size_t line = n + 1 - eol;
  if (line < 64) {
    return false;
  }
  size_t g = line, h = 64;
  while (h != 0) {
    const size_t t = g % h;
    g = h;
    h = t;
  }
  const size_t cycle = line / g;
  if (cycle > max_cycle) {
    return false;
  }
  p.cycle = cycle;
  p.eol = eol;
  const uint8_t eol_bytes[2] = {uint8_t(eol == 2 ? '\r' : '\n'), uint8_t('\n')};
  const uint8_t full_bytes[2] = {0xff, uint8_t(eol == 2 ? 0xff : 0)};
  uint16_t expect;
  uint16_t full;
  std::memcpy(&expect, eol_bytes, sizeof(expect));
  std::memcpy(&full, full_bytes, sizeof(full));
  size_t to_gap = line;
  for (size_t i = 0; i < cycle; i++) {
    step &s = p.steps[i];
    s.expect = expect;
    if (to_gap <= 64) {
      s.gap = uint8_t(to_gap);
      s.blend = to_gap == 64 ? 0 : ~uint64_t(0) << to_gap;
      s.advance = uint8_t(64 + eol);
      s.check = uint8_t(to_gap);
      s.mask = full;
      to_gap = line - (64 - to_gap);
    } else {
      s.gap = 64;
      s.blend = 0;
      s.advance = 64;
      s.check = 0;
      s.mask = 0;
      to_gap -= 64;
    }
  }
  for (size_t i = cycle; i < cycle + max_group - 1; i++) {
    p.steps[i] = p.steps[i % cycle];
  }
  return true;
}

} // namespace base64_lines
} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf
