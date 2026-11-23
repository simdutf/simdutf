namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {
namespace aligned_split {
constexpr size_t min_bytes = 65536;

template <typename T>
simdutf_really_inline size_t head(const T *p, size_t len) {
  if (len * sizeof(T) < min_bytes) {
    return 0;
  }
  const size_t misalignment = reinterpret_cast<uintptr_t>(p) % 64;
  if (misalignment == 0 || misalignment % sizeof(T) != 0) {
    return 0;
  }
  return (64 - misalignment) / sizeof(T);
}

#if SIMDUTF_FEATURE_UTF8 || SIMDUTF_FEATURE_DETECT_ENCODING
template <typename T>
simdutf_really_inline size_t utf8_head(const T *p, size_t len) {
  size_t h = head(p, len);
  if (h != 0) {
    for (int i = 0; i < 3 && h < len && (uint8_t(p[h]) & 0xC0) == 0x80; i++) {
      h++;
    }
  }
  return h;
}
#endif // SIMDUTF_FEATURE_UTF8 || SIMDUTF_FEATURE_DETECT_ENCODING

#if SIMDUTF_FEATURE_UTF16 || SIMDUTF_FEATURE_DETECT_ENCODING
template <endianness big_endian>
simdutf_really_inline size_t utf16_head(const char16_t *p, size_t len) {
  size_t h = head(p, len);
  if (h != 0 && h < len &&
      scalar::utf16::is_high_surrogate<big_endian>(p[h - 1]) &&
      scalar::utf16::is_low_surrogate<big_endian>(p[h])) {
    h++;
  }
  return h;
}
#endif // SIMDUTF_FEATURE_UTF16 || SIMDUTF_FEATURE_DETECT_ENCODING

template <auto F, typename... Args>
simdutf_never_inline auto out_of_line(Args... args) noexcept {
  return F(args...);
}

template <auto F, typename T>
simdutf_really_inline size_t sum(const T *p, size_t len, size_t h) {
  const size_t first = h == 0 ? 0 : out_of_line<F>(p, h);
  return first + F(p + h, len - h);
}

template <auto F, typename T>
simdutf_really_inline bool all(const T *p, size_t len, size_t h) {
  if (h != 0 && !out_of_line<F>(p, h)) {
    return false;
  }
  return F(p + h, len - h);
}

template <auto F, typename T>
simdutf_really_inline result first_error(const T *p, size_t len, size_t h) {
  if (h != 0) {
    const result first = out_of_line<F>(p, h);
    if (first.error != error_code::SUCCESS) {
      return first;
    }
  }
  result r = F(p + h, len - h);
  r.count += h;
  return r;
}

#if SIMDUTF_FEATURE_UTF8
template <auto F>
simdutf_really_inline utf8_result with_counts(const char *p, size_t len,
                                              size_t h) {
  utf8_result first;
  if (h != 0) {
    first = out_of_line<F>(p, h);
    if (first.error != error_code::SUCCESS) {
      return first;
    }
  }
  utf8_result r = F(p + h, len - h);
  r.input_count += h;
  r.continuation_count += first.continuation_count;
  r.four_byte_count += first.four_byte_count;
  return r;
}
#endif // SIMDUTF_FEATURE_UTF8

#if SIMDUTF_FEATURE_UTF16
template <auto F>
simdutf_really_inline result count_with_flag(const char16_t *p, size_t len,
                                             size_t h) {
  result first;
  if (h != 0) {
    first = out_of_line<F>(p, h);
  }
  result r = F(p + h, len - h);
  r.count += first.count;
  if (first.error != error_code::SUCCESS) {
    r.error = first.error;
  }
  return r;
}

template <auto F>
simdutf_really_inline void transform(const char16_t *in, size_t len,
                                     char16_t *out, size_t h) {
  if (h != 0) {
    out_of_line<F>(in, h, out);
  }
  F(in + h, len - h, out + h);
}
#endif // SIMDUTF_FEATURE_UTF16

} // namespace aligned_split
} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf
