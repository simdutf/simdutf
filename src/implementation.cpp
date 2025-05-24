#include "simdutf.h"
#include <initializer_list>
#include <climits>
#include <type_traits>
#if SIMDUTF_ATOMIC_REF
  #include "scalar/atomic_util.h"
#endif

static_assert(sizeof(uint8_t) == sizeof(char),
              "simdutf requires that uint8_t be a char");
static_assert(sizeof(uint16_t) == sizeof(char16_t),
              "simdutf requires that char16_t be 16 bits");
static_assert(sizeof(uint32_t) == sizeof(char32_t),
              "simdutf requires that char32_t be 32 bits");
// next line is redundant, but it is kept to catch defective systems.
static_assert(CHAR_BIT == 8, "simdutf requires 8-bit bytes");

// Useful for debugging purposes
namespace simdutf {
namespace {

template <typename T> std::string toBinaryString(T b) {
  std::string binary = "";
  T mask = T(1) << (sizeof(T) * CHAR_BIT - 1);
  while (mask > 0) {
    binary += ((b & mask) == 0) ? '0' : '1';
    mask >>= 1;
  }
  return binary;
}
} // namespace
} // namespace simdutf

namespace simdutf {
bool implementation::supported_by_runtime_system() const {
  uint32_t required_instruction_sets = this->required_instruction_sets();
  uint32_t supported_instruction_sets =
      internal::detect_supported_architectures();
  return ((supported_instruction_sets & required_instruction_sets) ==
          required_instruction_sets);
}

#if SIMDUTF_FEATURE_DETECT_ENCODING
simdutf_warn_unused encoding_type implementation::autodetect_encoding(
    const char *input, size_t length) const noexcept {
  // If there is a BOM, then we trust it.
  auto bom_encoding = simdutf::BOM::check_bom(input, length);
  if (bom_encoding != encoding_type::unspecified) {
    return bom_encoding;
  }
  // UTF8 is common, it includes ASCII, and is commonly represented
  // without a BOM, so if it fits, go with that. Note that it is still
  // possible to get it wrong, we are only 'guessing'. If some has UTF-16
  // data without a BOM, it could pass as UTF-8.
  //
  // An interesting twist might be to check for UTF-16 ASCII first (every
  // other byte is zero).
  if (validate_utf8(input, length)) {
    return encoding_type::UTF8;
  }
  // The next most common encoding that might appear without BOM is probably
  // UTF-16LE, so try that next.
  if ((length % 2) == 0) {
    // important: we need to divide by two
    if (validate_utf16le(reinterpret_cast<const char16_t *>(input),
                         length / 2)) {
      return encoding_type::UTF16_LE;
    }
  }
  if ((length % 4) == 0) {
    if (validate_utf32(reinterpret_cast<const char32_t *>(input), length / 4)) {
      return encoding_type::UTF32_LE;
    }
  }
  return encoding_type::unspecified;
}

  #ifdef SIMDUTF_INTERNAL_TESTS
std::vector<implementation::TestProcedure>
implementation::internal_tests() const {
  return {};
}
  #endif
#endif // SIMDUTF_FEATURE_DETECT_ENCODING

#if SIMDUTF_FEATURE_BASE64
simdutf_warn_unused size_t implementation::maximal_binary_length_from_base64(
    const char *input, size_t length) const noexcept {
  return scalar::base64::maximal_binary_length_from_base64(input, length);
}

simdutf_warn_unused size_t implementation::maximal_binary_length_from_base64(
    const char16_t *input, size_t length) const noexcept {
  return scalar::base64::maximal_binary_length_from_base64(input, length);
}

simdutf_warn_unused size_t implementation::base64_length_from_binary(
    size_t length, base64_options options) const noexcept {
  return scalar::base64::base64_length_from_binary(length, options);
}
#endif // SIMDUTF_FEATURE_BASE64

namespace internal {
// When there is a single implementation, we should not pay a price
// for dispatching to the best implementation. We should just use the
// one we have. This is a compile-time check.
#define SIMDUTF_SINGLE_IMPLEMENTATION                                          \
  (SIMDUTF_IMPLEMENTATION_ICELAKE + SIMDUTF_IMPLEMENTATION_HASWELL +           \
       SIMDUTF_IMPLEMENTATION_WESTMERE + SIMDUTF_IMPLEMENTATION_ARM64 +        \
       SIMDUTF_IMPLEMENTATION_PPC64 + SIMDUTF_IMPLEMENTATION_LSX +             \
       SIMDUTF_IMPLEMENTATION_LASX + SIMDUTF_IMPLEMENTATION_FALLBACK ==        \
   1)

// Static array of known implementations. We are hoping these get baked into the
// executable without requiring a static initializer.

#if SIMDUTF_IMPLEMENTATION_ICELAKE
static const icelake::implementation *get_icelake_singleton() {
  static const icelake::implementation icelake_singleton{};
  return &icelake_singleton;
}
#endif
#if SIMDUTF_IMPLEMENTATION_HASWELL
static const haswell::implementation *get_haswell_singleton() {
  static const haswell::implementation haswell_singleton{};
  return &haswell_singleton;
}
#endif
#if SIMDUTF_IMPLEMENTATION_WESTMERE
static const westmere::implementation *get_westmere_singleton() {
  static const westmere::implementation westmere_singleton{};
  return &westmere_singleton;
}
#endif
#if SIMDUTF_IMPLEMENTATION_ARM64
static const arm64::implementation *get_arm64_singleton() {
  static const arm64::implementation arm64_singleton{};
  return &arm64_singleton;
}
#endif
#if SIMDUTF_IMPLEMENTATION_PPC64
static const ppc64::implementation *get_ppc64_singleton() {
  static const ppc64::implementation ppc64_singleton{};
  return &ppc64_singleton;
}
#endif
#if SIMDUTF_IMPLEMENTATION_RVV
static const rvv::implementation *get_rvv_singleton() {
  static const rvv::implementation rvv_singleton{};
  return &rvv_singleton;
}
#endif
#if SIMDUTF_IMPLEMENTATION_LSX
static const lsx::implementation *get_lsx_singleton() {
  static const lsx::implementation lsx_singleton{};
  return &lsx_singleton;
}
#endif
#if SIMDUTF_IMPLEMENTATION_LASX
static const lasx::implementation *get_lasx_singleton() {
  static const lasx::implementation lasx_singleton{};
  return &lasx_singleton;
}
#endif
#if SIMDUTF_IMPLEMENTATION_FALLBACK
static const fallback::implementation *get_fallback_singleton() {
  static const fallback::implementation fallback_singleton{};
  return &fallback_singleton;
}
#endif

#if SIMDUTF_SINGLE_IMPLEMENTATION
static const implementation *get_single_implementation() {
  return
  #if SIMDUTF_IMPLEMENTATION_ICELAKE
      get_icelake_singleton();
  #endif
  #if SIMDUTF_IMPLEMENTATION_HASWELL
  get_haswell_singleton();
  #endif
  #if SIMDUTF_IMPLEMENTATION_WESTMERE
  get_westmere_singleton();
  #endif
  #if SIMDUTF_IMPLEMENTATION_ARM64
  get_arm64_singleton();
  #endif
  #if SIMDUTF_IMPLEMENTATION_PPC64
  get_ppc64_singleton();
  #endif
  #if SIMDUTF_IMPLEMENTATION_LSX
  get_lsx_singleton();
  #endif
  #if SIMDUTF_IMPLEMENTATION_LASX
  get_lasx_singleton();
  #endif
  #if SIMDUTF_IMPLEMENTATION_FALLBACK
  get_fallback_singleton();
  #endif
}
#endif

/**
 * @private Detects best supported implementation on first use, and sets it
 */
class detect_best_supported_implementation_on_first_use final
    : public implementation {
public:
  std::string name() const noexcept final { return set_best()->name(); }
  std::string description() const noexcept final {
    return set_best()->description();
  }
  uint32_t required_instruction_sets() const noexcept final {
    return set_best()->required_instruction_sets();
  }

#if SIMDUTF_FEATURE_DETECT_ENCODING
  simdutf_warn_unused int
  detect_encodings(const char *input, size_t length) const noexcept override {
    return set_best()->detect_encodings(input, length);
  }
#endif // SIMDUTF_FEATURE_DETECT_ENCODING

#if SIMDUTF_FEATURE_UTF8 || SIMDUTF_FEATURE_DETECT_ENCODING
  simdutf_warn_unused bool
  validate_utf8(const char *buf, size_t len) const noexcept final override {
    return set_best()->validate_utf8(buf, len);
  }
#endif // SIMDUTF_FEATURE_UTF8 || SIMDUTF_FEATURE_DETECT_ENCODING

#if SIMDUTF_FEATURE_UTF8
  simdutf_warn_unused result validate_utf8_with_errors(
      const char *buf, size_t len) const noexcept final override {
    return set_best()->validate_utf8_with_errors(buf, len);
  }
#endif // SIMDUTF_FEATURE_UTF8

#if SIMDUTF_FEATURE_ASCII
  simdutf_warn_unused bool
  validate_ascii(const char *buf, size_t len) const noexcept final override {
    return set_best()->validate_ascii(buf, len);
  }

  simdutf_warn_unused result validate_ascii_with_errors(
      const char *buf, size_t len) const noexcept final override {
    return set_best()->validate_ascii_with_errors(buf, len);
  }
#endif // SIMDUTF_FEATURE_ASCII

#if SIMDUTF_FEATURE_UTF16 || SIMDUTF_FEATURE_DETECT_ENCODING
  simdutf_warn_unused bool
  validate_utf16le(const char16_t *buf,
                   size_t len) const noexcept final override {
    return set_best()->validate_utf16le(buf, len);
  }
#endif // SIMDUTF_FEATURE_UTF16 || SIMDUTF_FEATURE_DETECT_ENCODING

#if SIMDUTF_FEATURE_UTF16
  simdutf_warn_unused bool
  validate_utf16be(const char16_t *buf,
                   size_t len) const noexcept final override {
    return set_best()->validate_utf16be(buf, len);
  }

  simdutf_warn_unused result validate_utf16le_with_errors(
      const char16_t *buf, size_t len) const noexcept final override {
    return set_best()->validate_utf16le_with_errors(buf, len);
  }

  simdutf_warn_unused result validate_utf16be_with_errors(
      const char16_t *buf, size_t len) const noexcept final override {
    return set_best()->validate_utf16be_with_errors(buf, len);
  }
  void to_well_formed_utf16be(const char16_t *input, size_t len,
                              char16_t *output) const noexcept final override {
    return set_best()->to_well_formed_utf16be(input, len, output);
  }
  void to_well_formed_utf16le(const char16_t *input, size_t len,
                              char16_t *output) const noexcept final override {
    return set_best()->to_well_formed_utf16le(input, len, output);
  }
#endif // SIMDUTF_FEATURE_UTF16

#if SIMDUTF_FEATURE_UTF32 || SIMDUTF_FEATURE_DETECT_ENCODING
  simdutf_warn_unused bool
  validate_utf32(const char32_t *buf,
                 size_t len) const noexcept final override {
    return set_best()->validate_utf32(buf, len);
  }
#endif // SIMDUTF_FEATURE_UTF32 || SIMDUTF_FEATURE_DETECT_ENCODING

#if SIMDUTF_FEATURE_UTF32
  simdutf_warn_unused result validate_utf32_with_errors(
      const char32_t *buf, size_t len) const noexcept final override {
    return set_best()->validate_utf32_with_errors(buf, len);
  }
#endif // SIMDUTF_FEATURE_UTF32

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1
  simdutf_warn_unused size_t
  convert_latin1_to_utf8(const char *buf, size_t len,
                         char *utf8_output) const noexcept final override {
    return set_best()->convert_latin1_to_utf8(buf, len, utf8_output);
  }
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1

#if SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_LATIN1
  simdutf_warn_unused size_t convert_latin1_to_utf16le(
      const char *buf, size_t len,
      char16_t *utf16_output) const noexcept final override {
    return set_best()->convert_latin1_to_utf16le(buf, len, utf16_output);
  }

  simdutf_warn_unused size_t convert_latin1_to_utf16be(
      const char *buf, size_t len,
      char16_t *utf16_output) const noexcept final override {
    return set_best()->convert_latin1_to_utf16be(buf, len, utf16_output);
  }
#endif // SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_LATIN1

#if SIMDUTF_FEATURE_UTF32 && SIMDUTF_FEATURE_LATIN1
  simdutf_warn_unused size_t convert_latin1_to_utf32(
      const char *buf, size_t len,
      char32_t *latin1_output) const noexcept final override {
    return set_best()->convert_latin1_to_utf32(buf, len, latin1_output);
  }
#endif // SIMDUTF_FEATURE_UTF32 && SIMDUTF_FEATURE_LATIN1

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1
  simdutf_warn_unused size_t
  convert_utf8_to_latin1(const char *buf, size_t len,
                         char *latin1_output) const noexcept final override {
    return set_best()->convert_utf8_to_latin1(buf, len, latin1_output);
  }

  simdutf_warn_unused result convert_utf8_to_latin1_with_errors(
      const char *buf, size_t len,
      char *latin1_output) const noexcept final override {
    return set_best()->convert_utf8_to_latin1_with_errors(buf, len,
                                                          latin1_output);
  }

  simdutf_warn_unused size_t convert_valid_utf8_to_latin1(
      const char *buf, size_t len,
      char *latin1_output) const noexcept final override {
    return set_best()->convert_valid_utf8_to_latin1(buf, len, latin1_output);
  }
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16
  simdutf_warn_unused size_t convert_utf8_to_utf16le(
      const char *buf, size_t len,
      char16_t *utf16_output) const noexcept final override {
    return set_best()->convert_utf8_to_utf16le(buf, len, utf16_output);
  }

  simdutf_warn_unused size_t convert_utf8_to_utf16be(
      const char *buf, size_t len,
      char16_t *utf16_output) const noexcept final override {
    return set_best()->convert_utf8_to_utf16be(buf, len, utf16_output);
  }

  simdutf_warn_unused result convert_utf8_to_utf16le_with_errors(
      const char *buf, size_t len,
      char16_t *utf16_output) const noexcept final override {
    return set_best()->convert_utf8_to_utf16le_with_errors(buf, len,
                                                           utf16_output);
  }

  simdutf_warn_unused result convert_utf8_to_utf16be_with_errors(
      const char *buf, size_t len,
      char16_t *utf16_output) const noexcept final override {
    return set_best()->convert_utf8_to_utf16be_with_errors(buf, len,
                                                           utf16_output);
  }

  simdutf_warn_unused size_t convert_valid_utf8_to_utf16le(
      const char *buf, size_t len,
      char16_t *utf16_output) const noexcept final override {
    return set_best()->convert_valid_utf8_to_utf16le(buf, len, utf16_output);
  }

  simdutf_warn_unused size_t convert_valid_utf8_to_utf16be(
      const char *buf, size_t len,
      char16_t *utf16_output) const noexcept final override {
    return set_best()->convert_valid_utf8_to_utf16be(buf, len, utf16_output);
  }
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32
  simdutf_warn_unused size_t
  convert_utf8_to_utf32(const char *buf, size_t len,
                        char32_t *utf32_output) const noexcept final override {
    return set_best()->convert_utf8_to_utf32(buf, len, utf32_output);
  }

  simdutf_warn_unused result convert_utf8_to_utf32_with_errors(
      const char *buf, size_t len,
      char32_t *utf32_output) const noexcept final override {
    return set_best()->convert_utf8_to_utf32_with_errors(buf, len,
                                                         utf32_output);
  }

  simdutf_warn_unused size_t convert_valid_utf8_to_utf32(
      const char *buf, size_t len,
      char32_t *utf32_output) const noexcept final override {
    return set_best()->convert_valid_utf8_to_utf32(buf, len, utf32_output);
  }
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32

#if SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_LATIN1
  simdutf_warn_unused size_t
  convert_utf16le_to_latin1(const char16_t *buf, size_t len,
                            char *latin1_output) const noexcept final override {
    return set_best()->convert_utf16le_to_latin1(buf, len, latin1_output);
  }

  simdutf_warn_unused size_t
  convert_utf16be_to_latin1(const char16_t *buf, size_t len,
                            char *latin1_output) const noexcept final override {
    return set_best()->convert_utf16be_to_latin1(buf, len, latin1_output);
  }

  simdutf_warn_unused result convert_utf16le_to_latin1_with_errors(
      const char16_t *buf, size_t len,
      char *latin1_output) const noexcept final override {
    return set_best()->convert_utf16le_to_latin1_with_errors(buf, len,
                                                             latin1_output);
  }

  simdutf_warn_unused result convert_utf16be_to_latin1_with_errors(
      const char16_t *buf, size_t len,
      char *latin1_output) const noexcept final override {
    return set_best()->convert_utf16be_to_latin1_with_errors(buf, len,
                                                             latin1_output);
  }

  simdutf_warn_unused size_t convert_valid_utf16le_to_latin1(
      const char16_t *buf, size_t len,
      char *latin1_output) const noexcept final override {
    return set_best()->convert_valid_utf16le_to_latin1(buf, len, latin1_output);
  }

  simdutf_warn_unused size_t convert_valid_utf16be_to_latin1(
      const char16_t *buf, size_t len,
      char *latin1_output) const noexcept final override {
    return set_best()->convert_valid_utf16be_to_latin1(buf, len, latin1_output);
  }
#endif // SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_LATIN1

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16
  simdutf_warn_unused size_t
  convert_utf16le_to_utf8(const char16_t *buf, size_t len,
                          char *utf8_output) const noexcept final override {
    return set_best()->convert_utf16le_to_utf8(buf, len, utf8_output);
  }

  simdutf_warn_unused size_t
  convert_utf16be_to_utf8(const char16_t *buf, size_t len,
                          char *utf8_output) const noexcept final override {
    return set_best()->convert_utf16be_to_utf8(buf, len, utf8_output);
  }

  simdutf_warn_unused result convert_utf16le_to_utf8_with_errors(
      const char16_t *buf, size_t len,
      char *utf8_output) const noexcept final override {
    return set_best()->convert_utf16le_to_utf8_with_errors(buf, len,
                                                           utf8_output);
  }

  simdutf_warn_unused result convert_utf16be_to_utf8_with_errors(
      const char16_t *buf, size_t len,
      char *utf8_output) const noexcept final override {
    return set_best()->convert_utf16be_to_utf8_with_errors(buf, len,
                                                           utf8_output);
  }

  simdutf_warn_unused size_t convert_valid_utf16le_to_utf8(
      const char16_t *buf, size_t len,
      char *utf8_output) const noexcept final override {
    return set_best()->convert_valid_utf16le_to_utf8(buf, len, utf8_output);
  }

  simdutf_warn_unused size_t convert_valid_utf16be_to_utf8(
      const char16_t *buf, size_t len,
      char *utf8_output) const noexcept final override {
    return set_best()->convert_valid_utf16be_to_utf8(buf, len, utf8_output);
  }
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16

#if SIMDUTF_FEATURE_UTF32 && SIMDUTF_FEATURE_LATIN1
  simdutf_warn_unused size_t
  convert_utf32_to_latin1(const char32_t *buf, size_t len,
                          char *latin1_output) const noexcept final override {
    return set_best()->convert_utf32_to_latin1(buf, len, latin1_output);
  }

  simdutf_warn_unused result convert_utf32_to_latin1_with_errors(
      const char32_t *buf, size_t len,
      char *latin1_output) const noexcept final override {
    return set_best()->convert_utf32_to_latin1_with_errors(buf, len,
                                                           latin1_output);
  }

  simdutf_warn_unused size_t convert_valid_utf32_to_latin1(
      const char32_t *buf, size_t len,
      char *latin1_output) const noexcept final override {
    return set_best()->convert_utf32_to_latin1(buf, len, latin1_output);
  }
#endif // SIMDUTF_FEATURE_UTF32 && SIMDUTF_FEATURE_LATIN1

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32
  simdutf_warn_unused size_t
  convert_utf32_to_utf8(const char32_t *buf, size_t len,
                        char *utf8_output) const noexcept final override {
    return set_best()->convert_utf32_to_utf8(buf, len, utf8_output);
  }

  simdutf_warn_unused result convert_utf32_to_utf8_with_errors(
      const char32_t *buf, size_t len,
      char *utf8_output) const noexcept final override {
    return set_best()->convert_utf32_to_utf8_with_errors(buf, len, utf8_output);
  }

  simdutf_warn_unused size_t
  convert_valid_utf32_to_utf8(const char32_t *buf, size_t len,
                              char *utf8_output) const noexcept final override {
    return set_best()->convert_valid_utf32_to_utf8(buf, len, utf8_output);
  }
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32

#if SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_UTF32
  simdutf_warn_unused size_t convert_utf32_to_utf16le(
      const char32_t *buf, size_t len,
      char16_t *utf16_output) const noexcept final override {
    return set_best()->convert_utf32_to_utf16le(buf, len, utf16_output);
  }

  simdutf_warn_unused size_t convert_utf32_to_utf16be(
      const char32_t *buf, size_t len,
      char16_t *utf16_output) const noexcept final override {
    return set_best()->convert_utf32_to_utf16be(buf, len, utf16_output);
  }

  simdutf_warn_unused result convert_utf32_to_utf16le_with_errors(
      const char32_t *buf, size_t len,
      char16_t *utf16_output) const noexcept final override {
    return set_best()->convert_utf32_to_utf16le_with_errors(buf, len,
                                                            utf16_output);
  }

  simdutf_warn_unused result convert_utf32_to_utf16be_with_errors(
      const char32_t *buf, size_t len,
      char16_t *utf16_output) const noexcept final override {
    return set_best()->convert_utf32_to_utf16be_with_errors(buf, len,
                                                            utf16_output);
  }

  simdutf_warn_unused size_t convert_valid_utf32_to_utf16le(
      const char32_t *buf, size_t len,
      char16_t *utf16_output) const noexcept final override {
    return set_best()->convert_valid_utf32_to_utf16le(buf, len, utf16_output);
  }

  simdutf_warn_unused size_t convert_valid_utf32_to_utf16be(
      const char32_t *buf, size_t len,
      char16_t *utf16_output) const noexcept final override {
    return set_best()->convert_valid_utf32_to_utf16be(buf, len, utf16_output);
  }

  simdutf_warn_unused size_t convert_utf16le_to_utf32(
      const char16_t *buf, size_t len,
      char32_t *utf32_output) const noexcept final override {
    return set_best()->convert_utf16le_to_utf32(buf, len, utf32_output);
  }

  simdutf_warn_unused size_t convert_utf16be_to_utf32(
      const char16_t *buf, size_t len,
      char32_t *utf32_output) const noexcept final override {
    return set_best()->convert_utf16be_to_utf32(buf, len, utf32_output);
  }

  simdutf_warn_unused result convert_utf16le_to_utf32_with_errors(
      const char16_t *buf, size_t len,
      char32_t *utf32_output) const noexcept final override {
    return set_best()->convert_utf16le_to_utf32_with_errors(buf, len,
                                                            utf32_output);
  }

  simdutf_warn_unused result convert_utf16be_to_utf32_with_errors(
      const char16_t *buf, size_t len,
      char32_t *utf32_output) const noexcept final override {
    return set_best()->convert_utf16be_to_utf32_with_errors(buf, len,
                                                            utf32_output);
  }

  simdutf_warn_unused size_t convert_valid_utf16le_to_utf32(
      const char16_t *buf, size_t len,
      char32_t *utf32_output) const noexcept final override {
    return set_best()->convert_valid_utf16le_to_utf32(buf, len, utf32_output);
  }

  simdutf_warn_unused size_t convert_valid_utf16be_to_utf32(
      const char16_t *buf, size_t len,
      char32_t *utf32_output) const noexcept final override {
    return set_best()->convert_valid_utf16be_to_utf32(buf, len, utf32_output);
  }
#endif // SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_UTF32

#if SIMDUTF_FEATURE_UTF16
  void change_endianness_utf16(const char16_t *buf, size_t len,
                               char16_t *output) const noexcept final override {
    set_best()->change_endianness_utf16(buf, len, output);
  }

  simdutf_warn_unused size_t
  count_utf16le(const char16_t *buf, size_t len) const noexcept final override {
    return set_best()->count_utf16le(buf, len);
  }

  simdutf_warn_unused size_t
  count_utf16be(const char16_t *buf, size_t len) const noexcept final override {
    return set_best()->count_utf16be(buf, len);
  }
#endif // SIMDUTF_FEATURE_UTF16

#if SIMDUTF_FEATURE_UTF8
  simdutf_warn_unused size_t
  count_utf8(const char *buf, size_t len) const noexcept final override {
    return set_best()->count_utf8(buf, len);
  }
#endif // SIMDUTF_FEATURE_UTF8

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1
  simdutf_warn_unused size_t
  latin1_length_from_utf8(const char *buf, size_t len) const noexcept override {
    return set_best()->latin1_length_from_utf8(buf, len);
  }
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1
  simdutf_warn_unused size_t
  utf8_length_from_latin1(const char *buf, size_t len) const noexcept override {
    return set_best()->utf8_length_from_latin1(buf, len);
  }
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16
  simdutf_warn_unused size_t utf8_length_from_utf16le(
      const char16_t *buf, size_t len) const noexcept override {
    return set_best()->utf8_length_from_utf16le(buf, len);
  }

  simdutf_warn_unused size_t utf8_length_from_utf16be(
      const char16_t *buf, size_t len) const noexcept override {
    return set_best()->utf8_length_from_utf16be(buf, len);
  }
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16

#if SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_UTF32
  simdutf_warn_unused size_t utf32_length_from_utf16le(
      const char16_t *buf, size_t len) const noexcept override {
    return set_best()->utf32_length_from_utf16le(buf, len);
  }

  simdutf_warn_unused size_t utf32_length_from_utf16be(
      const char16_t *buf, size_t len) const noexcept override {
    return set_best()->utf32_length_from_utf16be(buf, len);
  }
#endif // SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_UTF32

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16
  simdutf_warn_unused size_t
  utf16_length_from_utf8(const char *buf, size_t len) const noexcept override {
    return set_best()->utf16_length_from_utf8(buf, len);
  }
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32
  simdutf_warn_unused size_t utf8_length_from_utf32(
      const char32_t *buf, size_t len) const noexcept override {
    return set_best()->utf8_length_from_utf32(buf, len);
  }
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32

#if SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_UTF32
  simdutf_warn_unused size_t utf16_length_from_utf32(
      const char32_t *buf, size_t len) const noexcept override {
    return set_best()->utf16_length_from_utf32(buf, len);
  }
#endif // SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_UTF32

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32
  simdutf_warn_unused size_t
  utf32_length_from_utf8(const char *buf, size_t len) const noexcept override {
    return set_best()->utf32_length_from_utf8(buf, len);
  }
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32

#if SIMDUTF_FEATURE_BASE64
  simdutf_warn_unused result base64_to_binary(
      const char *input, size_t length, char *output, base64_options options,
      last_chunk_handling_options last_chunk_handling_options =
          last_chunk_handling_options::loose) const noexcept override {
    return set_best()->base64_to_binary(input, length, output, options,
                                        last_chunk_handling_options);
  }

  simdutf_warn_unused full_result base64_to_binary_details(
      const char *input, size_t length, char *output, base64_options options,
      last_chunk_handling_options last_chunk_handling_options =
          last_chunk_handling_options::loose) const noexcept override {
    return set_best()->base64_to_binary_details(input, length, output, options,
                                                last_chunk_handling_options);
  }

  simdutf_warn_unused result base64_to_binary(
      const char16_t *input, size_t length, char *output,
      base64_options options,
      last_chunk_handling_options last_chunk_handling_options =
          last_chunk_handling_options::loose) const noexcept override {
    return set_best()->base64_to_binary(input, length, output, options,
                                        last_chunk_handling_options);
  }

  simdutf_warn_unused full_result base64_to_binary_details(
      const char16_t *input, size_t length, char *output,
      base64_options options,
      last_chunk_handling_options last_chunk_handling_options =
          last_chunk_handling_options::loose) const noexcept override {
    return set_best()->base64_to_binary_details(input, length, output, options,
                                                last_chunk_handling_options);
  }

  size_t binary_to_base64(const char *input, size_t length, char *output,
                          base64_options options) const noexcept override {
    return set_best()->binary_to_base64(input, length, output, options);
  }

  const char *find(const char *start, const char *end,
                   char character) const noexcept override {
    return set_best()->find(start, end, character);
  }

  const char16_t *find(const char16_t *start, const char16_t *end,
                       char16_t character) const noexcept override {
    return set_best()->find(start, end, character);
  }
#endif // SIMDUTF_FEATURE_BASE64

  simdutf_really_inline
  detect_best_supported_implementation_on_first_use() noexcept
      : implementation("best_supported_detector",
                       "Detects the best supported implementation and sets it",
                       0) {}

private:
  const implementation *set_best() const noexcept;
};

static_assert(std::is_trivially_destructible<
                  detect_best_supported_implementation_on_first_use>::value,
              "detect_best_supported_implementation_on_first_use should be "
              "trivially destructible");

static const std::initializer_list<const implementation *> &
get_available_implementation_pointers() {
  static const std::initializer_list<const implementation *>
      available_implementation_pointers{
#if SIMDUTF_IMPLEMENTATION_ICELAKE
          get_icelake_singleton(),
#endif
#if SIMDUTF_IMPLEMENTATION_HASWELL
          get_haswell_singleton(),
#endif
#if SIMDUTF_IMPLEMENTATION_WESTMERE
          get_westmere_singleton(),
#endif
#if SIMDUTF_IMPLEMENTATION_ARM64
          get_arm64_singleton(),
#endif
#if SIMDUTF_IMPLEMENTATION_PPC64
          get_ppc64_singleton(),
#endif
#if SIMDUTF_IMPLEMENTATION_RVV
          get_rvv_singleton(),
#endif
#if SIMDUTF_IMPLEMENTATION_LSX
          get_lsx_singleton(),
#endif
#if SIMDUTF_IMPLEMENTATION_LASX
          get_lasx_singleton(),
#endif
#if SIMDUTF_IMPLEMENTATION_FALLBACK
          get_fallback_singleton(),
#endif
      }; // available_implementation_pointers
  return available_implementation_pointers;
}

// So we can return UNSUPPORTED_ARCHITECTURE from the parser when there is no
// support
class unsupported_implementation final : public implementation {
public:
#if SIMDUTF_FEATURE_DETECT_ENCODING
  simdutf_warn_unused int detect_encodings(const char *,
                                           size_t) const noexcept override {
    return encoding_type::unspecified;
  }
#endif // SIMDUTF_FEATURE_DETECT_ENCODING

#if SIMDUTF_FEATURE_UTF8 || SIMDUTF_FEATURE_DETECT_ENCODING
  simdutf_warn_unused bool validate_utf8(const char *,
                                         size_t) const noexcept final override {
    return false; // Just refuse to validate. Given that we have a fallback
                  // implementation
    // it seems unlikely that unsupported_implementation will ever be used. If
    // it is used, then it will flag all strings as invalid. The alternative is
    // to return an error_code from which the user has to figure out whether the
    // string is valid UTF-8... which seems like a lot of work just to handle
    // the very unlikely case that we have an unsupported implementation. And,
    // when it does happen (that we have an unsupported implementation), what
    // are the chances that the programmer has a fallback? Given that *we*
    // provide the fallback, it implies that the programmer would need a
    // fallback for our fallback.
  }
#endif // SIMDUTF_FEATURE_UTF8 || SIMDUTF_FEATURE_DETECT_ENCODING

#if SIMDUTF_FEATURE_UTF8
  simdutf_warn_unused result validate_utf8_with_errors(
      const char *, size_t) const noexcept final override {
    return result(error_code::OTHER, 0);
  }
#endif // SIMDUTF_FEATURE_UTF8

#if SIMDUTF_FEATURE_ASCII
  simdutf_warn_unused bool
  validate_ascii(const char *, size_t) const noexcept final override {
    return false;
  }

  simdutf_warn_unused result validate_ascii_with_errors(
      const char *, size_t) const noexcept final override {
    return result(error_code::OTHER, 0);
  }
#endif // SIMDUTF_FEATURE_ASCII

#if SIMDUTF_FEATURE_UTF16 || SIMDUTF_FEATURE_DETECT_ENCODING
  simdutf_warn_unused bool
  validate_utf16le(const char16_t *, size_t) const noexcept final override {
    return false;
  }
#endif // SIMDUTF_FEATURE_UTF16 || SIMDUTF_FEATURE_DETECT_ENCODING

#if SIMDUTF_FEATURE_UTF16
  simdutf_warn_unused bool
  validate_utf16be(const char16_t *, size_t) const noexcept final override {
    return false;
  }

  simdutf_warn_unused result validate_utf16le_with_errors(
      const char16_t *, size_t) const noexcept final override {
    return result(error_code::OTHER, 0);
  }

  simdutf_warn_unused result validate_utf16be_with_errors(
      const char16_t *, size_t) const noexcept final override {
    return result(error_code::OTHER, 0);
  }
  void to_well_formed_utf16be(const char16_t *, size_t,
                              char16_t *) const noexcept final override {}
  void to_well_formed_utf16le(const char16_t *, size_t,
                              char16_t *) const noexcept final override {}
#endif // SIMDUTF_FEATURE_UTF16

#if SIMDUTF_FEATURE_UTF32 || SIMDUTF_FEATURE_DETECT_ENCODING
  simdutf_warn_unused bool
  validate_utf32(const char32_t *, size_t) const noexcept final override {
    return false;
  }
#endif // SIMDUTF_FEATURE_UTF32 || SIMDUTF_FEATURE_DETECT_ENCODING

#if SIMDUTF_FEATURE_UTF32
  simdutf_warn_unused result validate_utf32_with_errors(
      const char32_t *, size_t) const noexcept final override {
    return result(error_code::OTHER, 0);
  }
#endif // SIMDUTF_FEATURE_UTF32

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1
  simdutf_warn_unused size_t convert_latin1_to_utf8(
      const char *, size_t, char *) const noexcept final override {
    return 0;
  }
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1

#if SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_LATIN1
  simdutf_warn_unused size_t convert_latin1_to_utf16le(
      const char *, size_t, char16_t *) const noexcept final override {
    return 0;
  }

  simdutf_warn_unused size_t convert_latin1_to_utf16be(
      const char *, size_t, char16_t *) const noexcept final override {
    return 0;
  }
#endif // SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_LATIN1

#if SIMDUTF_FEATURE_UTF32 && SIMDUTF_FEATURE_LATIN1
  simdutf_warn_unused size_t convert_latin1_to_utf32(
      const char *, size_t, char32_t *) const noexcept final override {
    return 0;
  }
#endif // SIMDUTF_FEATURE_UTF32 && SIMDUTF_FEATURE_LATIN1

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1
  simdutf_warn_unused size_t convert_utf8_to_latin1(
      const char *, size_t, char *) const noexcept final override {
    return 0;
  }

  simdutf_warn_unused result convert_utf8_to_latin1_with_errors(
      const char *, size_t, char *) const noexcept final override {
    return result(error_code::OTHER, 0);
  }

  simdutf_warn_unused size_t convert_valid_utf8_to_latin1(
      const char *, size_t, char *) const noexcept final override {
    return 0;
  }
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16
  simdutf_warn_unused size_t convert_utf8_to_utf16le(
      const char *, size_t, char16_t *) const noexcept final override {
    return 0;
  }

  simdutf_warn_unused size_t convert_utf8_to_utf16be(
      const char *, size_t, char16_t *) const noexcept final override {
    return 0;
  }

  simdutf_warn_unused result convert_utf8_to_utf16le_with_errors(
      const char *, size_t, char16_t *) const noexcept final override {
    return result(error_code::OTHER, 0);
  }

  simdutf_warn_unused result convert_utf8_to_utf16be_with_errors(
      const char *, size_t, char16_t *) const noexcept final override {
    return result(error_code::OTHER, 0);
  }

  simdutf_warn_unused size_t convert_valid_utf8_to_utf16le(
      const char *, size_t, char16_t *) const noexcept final override {
    return 0;
  }

  simdutf_warn_unused size_t convert_valid_utf8_to_utf16be(
      const char *, size_t, char16_t *) const noexcept final override {
    return 0;
  }
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32
  simdutf_warn_unused size_t convert_utf8_to_utf32(
      const char *, size_t, char32_t *) const noexcept final override {
    return 0;
  }

  simdutf_warn_unused result convert_utf8_to_utf32_with_errors(
      const char *, size_t, char32_t *) const noexcept final override {
    return result(error_code::OTHER, 0);
  }

  simdutf_warn_unused size_t convert_valid_utf8_to_utf32(
      const char *, size_t, char32_t *) const noexcept final override {
    return 0;
  }
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32

#if SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_LATIN1
  simdutf_warn_unused size_t convert_utf16le_to_latin1(
      const char16_t *, size_t, char *) const noexcept final override {
    return 0;
  }

  simdutf_warn_unused size_t convert_utf16be_to_latin1(
      const char16_t *, size_t, char *) const noexcept final override {
    return 0;
  }

  simdutf_warn_unused result convert_utf16le_to_latin1_with_errors(
      const char16_t *, size_t, char *) const noexcept final override {
    return result(error_code::OTHER, 0);
  }

  simdutf_warn_unused result convert_utf16be_to_latin1_with_errors(
      const char16_t *, size_t, char *) const noexcept final override {
    return result(error_code::OTHER, 0);
  }

  simdutf_warn_unused size_t convert_valid_utf16le_to_latin1(
      const char16_t *, size_t, char *) const noexcept final override {
    return 0;
  }

  simdutf_warn_unused size_t convert_valid_utf16be_to_latin1(
      const char16_t *, size_t, char *) const noexcept final override {
    return 0;
  }
#endif // SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_LATIN1

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16
  simdutf_warn_unused size_t convert_utf16le_to_utf8(
      const char16_t *, size_t, char *) const noexcept final override {
    return 0;
  }

  simdutf_warn_unused size_t convert_utf16be_to_utf8(
      const char16_t *, size_t, char *) const noexcept final override {
    return 0;
  }

  simdutf_warn_unused result convert_utf16le_to_utf8_with_errors(
      const char16_t *, size_t, char *) const noexcept final override {
    return result(error_code::OTHER, 0);
  }

  simdutf_warn_unused result convert_utf16be_to_utf8_with_errors(
      const char16_t *, size_t, char *) const noexcept final override {
    return result(error_code::OTHER, 0);
  }

  simdutf_warn_unused size_t convert_valid_utf16le_to_utf8(
      const char16_t *, size_t, char *) const noexcept final override {
    return 0;
  }

  simdutf_warn_unused size_t convert_valid_utf16be_to_utf8(
      const char16_t *, size_t, char *) const noexcept final override {
    return 0;
  }
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16

#if SIMDUTF_FEATURE_UTF32 && SIMDUTF_FEATURE_LATIN1
  simdutf_warn_unused size_t convert_utf32_to_latin1(
      const char32_t *, size_t, char *) const noexcept final override {
    return 0;
  }

  simdutf_warn_unused result convert_utf32_to_latin1_with_errors(
      const char32_t *, size_t, char *) const noexcept final override {
    return result(error_code::OTHER, 0);
  }

  simdutf_warn_unused size_t convert_valid_utf32_to_latin1(
      const char32_t *, size_t, char *) const noexcept final override {
    return 0;
  }
#endif // SIMDUTF_FEATURE_UTF32 && SIMDUTF_FEATURE_LATIN1

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32
  simdutf_warn_unused size_t convert_utf32_to_utf8(
      const char32_t *, size_t, char *) const noexcept final override {
    return 0;
  }

  simdutf_warn_unused result convert_utf32_to_utf8_with_errors(
      const char32_t *, size_t, char *) const noexcept final override {
    return result(error_code::OTHER, 0);
  }

  simdutf_warn_unused size_t convert_valid_utf32_to_utf8(
      const char32_t *, size_t, char *) const noexcept final override {
    return 0;
  }
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32

#if SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_UTF32
  simdutf_warn_unused size_t convert_utf32_to_utf16le(
      const char32_t *, size_t, char16_t *) const noexcept final override {
    return 0;
  }

  simdutf_warn_unused size_t convert_utf32_to_utf16be(
      const char32_t *, size_t, char16_t *) const noexcept final override {
    return 0;
  }

  simdutf_warn_unused result convert_utf32_to_utf16le_with_errors(
      const char32_t *, size_t, char16_t *) const noexcept final override {
    return result(error_code::OTHER, 0);
  }

  simdutf_warn_unused result convert_utf32_to_utf16be_with_errors(
      const char32_t *, size_t, char16_t *) const noexcept final override {
    return result(error_code::OTHER, 0);
  }

  simdutf_warn_unused size_t convert_valid_utf32_to_utf16le(
      const char32_t *, size_t, char16_t *) const noexcept final override {
    return 0;
  }

  simdutf_warn_unused size_t convert_valid_utf32_to_utf16be(
      const char32_t *, size_t, char16_t *) const noexcept final override {
    return 0;
  }

  simdutf_warn_unused size_t convert_utf16le_to_utf32(
      const char16_t *, size_t, char32_t *) const noexcept final override {
    return 0;
  }

  simdutf_warn_unused size_t convert_utf16be_to_utf32(
      const char16_t *, size_t, char32_t *) const noexcept final override {
    return 0;
  }

  simdutf_warn_unused result convert_utf16le_to_utf32_with_errors(
      const char16_t *, size_t, char32_t *) const noexcept final override {
    return result(error_code::OTHER, 0);
  }

  simdutf_warn_unused result convert_utf16be_to_utf32_with_errors(
      const char16_t *, size_t, char32_t *) const noexcept final override {
    return result(error_code::OTHER, 0);
  }

  simdutf_warn_unused size_t convert_valid_utf16le_to_utf32(
      const char16_t *, size_t, char32_t *) const noexcept final override {
    return 0;
  }

  simdutf_warn_unused size_t convert_valid_utf16be_to_utf32(
      const char16_t *, size_t, char32_t *) const noexcept final override {
    return 0;
  }
#endif // SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_UTF32

#if SIMDUTF_FEATURE_UTF16
  void change_endianness_utf16(const char16_t *, size_t,
                               char16_t *) const noexcept final override {}

  simdutf_warn_unused size_t
  count_utf16le(const char16_t *, size_t) const noexcept final override {
    return 0;
  }

  simdutf_warn_unused size_t
  count_utf16be(const char16_t *, size_t) const noexcept final override {
    return 0;
  }
#endif // SIMDUTF_FEATURE_UTF16

#if SIMDUTF_FEATURE_UTF8
  simdutf_warn_unused size_t count_utf8(const char *,
                                        size_t) const noexcept final override {
    return 0;
  }
#endif // SIMDUTF_FEATURE_UTF8

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1
  simdutf_warn_unused size_t
  latin1_length_from_utf8(const char *, size_t) const noexcept override {
    return 0;
  }
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1
  simdutf_warn_unused size_t
  utf8_length_from_latin1(const char *, size_t) const noexcept override {
    return 0;
  }
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16
  simdutf_warn_unused size_t
  utf8_length_from_utf16le(const char16_t *, size_t) const noexcept override {
    return 0;
  }

  simdutf_warn_unused size_t
  utf8_length_from_utf16be(const char16_t *, size_t) const noexcept override {
    return 0;
  }
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16

#if SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_UTF32
  simdutf_warn_unused size_t
  utf32_length_from_utf16le(const char16_t *, size_t) const noexcept override {
    return 0;
  }

  simdutf_warn_unused size_t
  utf32_length_from_utf16be(const char16_t *, size_t) const noexcept override {
    return 0;
  }
#endif // SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_UTF32

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16
  simdutf_warn_unused size_t
  utf16_length_from_utf8(const char *, size_t) const noexcept override {
    return 0;
  }
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32
  simdutf_warn_unused size_t
  utf8_length_from_utf32(const char32_t *, size_t) const noexcept override {
    return 0;
  }
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32

#if SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_UTF32
  simdutf_warn_unused size_t
  utf16_length_from_utf32(const char32_t *, size_t) const noexcept override {
    return 0;
  }
#endif // SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_UTF32

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32
  simdutf_warn_unused size_t
  utf32_length_from_utf8(const char *, size_t) const noexcept override {
    return 0;
  }
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32

#if SIMDUTF_FEATURE_BASE64
  simdutf_warn_unused result
  base64_to_binary(const char *, size_t, char *, base64_options,
                   last_chunk_handling_options) const noexcept override {
    return result(error_code::OTHER, 0);
  }

  simdutf_warn_unused full_result base64_to_binary_details(
      const char *, size_t, char *, base64_options,
      last_chunk_handling_options) const noexcept override {
    return full_result(error_code::OTHER, 0, 0);
  }

  simdutf_warn_unused result
  base64_to_binary(const char16_t *, size_t, char *, base64_options,
                   last_chunk_handling_options) const noexcept override {
    return result(error_code::OTHER, 0);
  }

  simdutf_warn_unused full_result base64_to_binary_details(
      const char16_t *, size_t, char *, base64_options,
      last_chunk_handling_options) const noexcept override {
    return full_result(error_code::OTHER, 0, 0);
  }

  size_t binary_to_base64(const char *, size_t, char *,
                          base64_options) const noexcept override {
    return 0;
  }
  const char *find(const char *, const char *, char) const noexcept override {
    return nullptr;
  }
  const char16_t *find(const char16_t *, const char16_t *,
                       char16_t) const noexcept override {
    return nullptr;
  }
#endif // SIMDUTF_FEATURE_BASE64

  unsupported_implementation()
      : implementation("unsupported",
                       "Unsupported CPU (no detected SIMD instructions)", 0) {}
};

const unsupported_implementation *get_unsupported_singleton() {
  static const unsupported_implementation unsupported_singleton{};
  return &unsupported_singleton;
}
static_assert(std::is_trivially_destructible<unsupported_implementation>::value,
              "unsupported_singleton should be trivially destructible");

size_t available_implementation_list::size() const noexcept {
  return internal::get_available_implementation_pointers().size();
}
const implementation *const *
available_implementation_list::begin() const noexcept {
  return internal::get_available_implementation_pointers().begin();
}
const implementation *const *
available_implementation_list::end() const noexcept {
  return internal::get_available_implementation_pointers().end();
}
const implementation *
available_implementation_list::detect_best_supported() const noexcept {
  // They are prelisted in priority order, so we just go down the list
  uint32_t supported_instruction_sets =
      internal::detect_supported_architectures();
  for (const implementation *impl :
       internal::get_available_implementation_pointers()) {
    uint32_t required_instruction_sets = impl->required_instruction_sets();
    if ((supported_instruction_sets & required_instruction_sets) ==
        required_instruction_sets) {
      return impl;
    }
  }
  return get_unsupported_singleton(); // this should never happen?
}

const implementation *
detect_best_supported_implementation_on_first_use::set_best() const noexcept {
  SIMDUTF_PUSH_DISABLE_WARNINGS
  SIMDUTF_DISABLE_DEPRECATED_WARNING // Disable CRT_SECURE warning on MSVC:
                                     // manually verified this is safe
      char *force_implementation_name = getenv("SIMDUTF_FORCE_IMPLEMENTATION");
  SIMDUTF_POP_DISABLE_WARNINGS

  if (force_implementation_name) {
    auto force_implementation =
        get_available_implementations()[force_implementation_name];
    if (force_implementation) {
      return get_active_implementation() = force_implementation;
    } else {
      // Note: abort() and stderr usage within the library is forbidden.
      return get_active_implementation() = get_unsupported_singleton();
    }
  }
  return get_active_implementation() =
             get_available_implementations().detect_best_supported();
}

} // namespace internal

/**
 * The list of available implementations compiled into simdutf.
 */
SIMDUTF_DLLIMPORTEXPORT const internal::available_implementation_list &
get_available_implementations() {
  static const internal::available_implementation_list
      available_implementations{};
  return available_implementations;
}

/**
 * The active implementation.
 */
SIMDUTF_DLLIMPORTEXPORT internal::atomic_ptr<const implementation> &
get_active_implementation() {
#if SIMDUTF_SINGLE_IMPLEMENTATION
  // skip runtime detection
  static internal::atomic_ptr<const implementation> active_implementation{
      internal::get_single_implementation()};
  return active_implementation;
#else
  static const internal::detect_best_supported_implementation_on_first_use
      detect_best_supported_implementation_on_first_use_singleton;
  static internal::atomic_ptr<const implementation> active_implementation{
      &detect_best_supported_implementation_on_first_use_singleton};
  return active_implementation;
#endif
}

#if SIMDUTF_SINGLE_IMPLEMENTATION
const implementation *get_default_implementation() {
  return internal::get_single_implementation();
}
#else
internal::atomic_ptr<const implementation> &get_default_implementation() {
  return get_active_implementation();
}
#endif
#define SIMDUTF_GET_CURRENT_IMPLEMENTION

#if SIMDUTF_FEATURE_UTF8
simdutf_warn_unused bool validate_utf8(const char *buf, size_t len) noexcept {
  return get_default_implementation()->validate_utf8(buf, len);
}
simdutf_warn_unused result validate_utf8_with_errors(const char *buf,
                                                     size_t len) noexcept {
  return get_default_implementation()->validate_utf8_with_errors(buf, len);
}
#endif // SIMDUTF_FEATURE_UTF8

#if SIMDUTF_FEATURE_ASCII
simdutf_warn_unused bool validate_ascii(const char *buf, size_t len) noexcept {
  return get_default_implementation()->validate_ascii(buf, len);
}
simdutf_warn_unused result validate_ascii_with_errors(const char *buf,
                                                      size_t len) noexcept {
  return get_default_implementation()->validate_ascii_with_errors(buf, len);
}
#endif // SIMDUTF_FEATURE_ASCII

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16
simdutf_warn_unused size_t convert_utf8_to_utf16(
    const char *input, size_t length, char16_t *utf16_output) noexcept {
  #if SIMDUTF_IS_BIG_ENDIAN
  return convert_utf8_to_utf16be(input, length, utf16_output);
  #else
  return convert_utf8_to_utf16le(input, length, utf16_output);
  #endif
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1
simdutf_warn_unused size_t convert_latin1_to_utf8(const char *buf, size_t len,
                                                  char *utf8_output) noexcept {
  return get_default_implementation()->convert_latin1_to_utf8(buf, len,
                                                              utf8_output);
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1

#if SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_LATIN1
simdutf_warn_unused size_t convert_latin1_to_utf16le(
    const char *buf, size_t len, char16_t *utf16_output) noexcept {
  return get_default_implementation()->convert_latin1_to_utf16le(buf, len,
                                                                 utf16_output);
}
simdutf_warn_unused size_t convert_latin1_to_utf16be(
    const char *buf, size_t len, char16_t *utf16_output) noexcept {
  return get_default_implementation()->convert_latin1_to_utf16be(buf, len,
                                                                 utf16_output);
}
#endif // SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_LATIN1

#if SIMDUTF_FEATURE_UTF32 && SIMDUTF_FEATURE_LATIN1
simdutf_warn_unused size_t convert_latin1_to_utf32(
    const char *buf, size_t len, char32_t *latin1_output) noexcept {
  return get_default_implementation()->convert_latin1_to_utf32(buf, len,
                                                               latin1_output);
}
simdutf_warn_unused size_t latin1_length_from_utf32(size_t length) noexcept {
  return length;
}
simdutf_warn_unused size_t utf32_length_from_latin1(size_t length) noexcept {
  return length;
}
#endif // SIMDUTF_FEATURE_UTF32 && SIMDUTF_FEATURE_LATIN1

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1
simdutf_warn_unused size_t convert_utf8_to_latin1(
    const char *buf, size_t len, char *latin1_output) noexcept {
  return get_default_implementation()->convert_utf8_to_latin1(buf, len,
                                                              latin1_output);
}
simdutf_warn_unused result convert_utf8_to_latin1_with_errors(
    const char *buf, size_t len, char *latin1_output) noexcept {
  return get_default_implementation()->convert_utf8_to_latin1_with_errors(
      buf, len, latin1_output);
}
simdutf_warn_unused size_t convert_valid_utf8_to_latin1(
    const char *buf, size_t len, char *latin1_output) noexcept {
  return get_default_implementation()->convert_valid_utf8_to_latin1(
      buf, len, latin1_output);
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16
simdutf_warn_unused size_t convert_utf8_to_utf16le(
    const char *input, size_t length, char16_t *utf16_output) noexcept {
  return get_default_implementation()->convert_utf8_to_utf16le(input, length,
                                                               utf16_output);
}
simdutf_warn_unused size_t convert_utf8_to_utf16be(
    const char *input, size_t length, char16_t *utf16_output) noexcept {
  return get_default_implementation()->convert_utf8_to_utf16be(input, length,
                                                               utf16_output);
}
simdutf_warn_unused result convert_utf8_to_utf16_with_errors(
    const char *input, size_t length, char16_t *utf16_output) noexcept {
  #if SIMDUTF_IS_BIG_ENDIAN
  return convert_utf8_to_utf16be_with_errors(input, length, utf16_output);
  #else
  return convert_utf8_to_utf16le_with_errors(input, length, utf16_output);
  #endif
}
simdutf_warn_unused result convert_utf8_to_utf16le_with_errors(
    const char *input, size_t length, char16_t *utf16_output) noexcept {
  return get_default_implementation()->convert_utf8_to_utf16le_with_errors(
      input, length, utf16_output);
}
simdutf_warn_unused result convert_utf8_to_utf16be_with_errors(
    const char *input, size_t length, char16_t *utf16_output) noexcept {
  return get_default_implementation()->convert_utf8_to_utf16be_with_errors(
      input, length, utf16_output);
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32
simdutf_warn_unused size_t convert_utf8_to_utf32(
    const char *input, size_t length, char32_t *utf32_output) noexcept {
  return get_default_implementation()->convert_utf8_to_utf32(input, length,
                                                             utf32_output);
}
simdutf_warn_unused result convert_utf8_to_utf32_with_errors(
    const char *input, size_t length, char32_t *utf32_output) noexcept {
  return get_default_implementation()->convert_utf8_to_utf32_with_errors(
      input, length, utf32_output);
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32

#if SIMDUTF_FEATURE_UTF16
simdutf_warn_unused bool validate_utf16(const char16_t *buf,
                                        size_t len) noexcept {
  #if SIMDUTF_IS_BIG_ENDIAN
  return validate_utf16be(buf, len);
  #else
  return validate_utf16le(buf, len);
  #endif
}
void to_well_formed_utf16be(const char16_t *input, size_t len,
                            char16_t *output) noexcept {
  return get_default_implementation()->to_well_formed_utf16be(input, len,
                                                              output);
}
void to_well_formed_utf16le(const char16_t *input, size_t len,
                            char16_t *output) noexcept {
  return get_default_implementation()->to_well_formed_utf16le(input, len,
                                                              output);
}
void to_well_formed_utf16(const char16_t *input, size_t len,
                          char16_t *output) noexcept {
  #if SIMDUTF_IS_BIG_ENDIAN
  to_well_formed_utf16be(input, len, output);
  #else
  to_well_formed_utf16le(input, len, output);
  #endif
}
#endif // SIMDUTF_FEATURE_UTF16

#if SIMDUTF_FEATURE_UTF16 || SIMDUTF_FEATURE_DETECT_ENCODING
simdutf_warn_unused bool validate_utf16le(const char16_t *buf,
                                          size_t len) noexcept {
  return get_default_implementation()->validate_utf16le(buf, len);
}

  #if SIMDUTF_ATOMIC_REF
template <typename char_type>
simdutf_warn_unused result atomic_base64_to_binary_safe_impl(
    const char_type *input, size_t length, char *output, size_t &outlen,
    base64_options options,
    last_chunk_handling_options last_chunk_handling_options,
    bool decode_up_to_bad_char) noexcept {
    #if defined(FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION)
  // We use a smaller buffer during fuzzing to more easily detect bugs.
  constexpr size_t buffer_size = 128;
    #else
  // Arbitrary block sizes: 4KB for input.
  constexpr size_t buffer_size = 4096;
    #endif
  std::array<char, buffer_size> temp_buffer;
  const char_type *const input_init = input;
  size_t actual_out = 0;
  bool last_chunk = false;
  result r;
  while (!last_chunk) {
    last_chunk |= (temp_buffer.size() >= outlen - actual_out);
    size_t temp_outlen = (std::min)(temp_buffer.size(), outlen - actual_out);
    r = base64_to_binary_safe(input, length, temp_buffer.data(), temp_outlen,
                              options, last_chunk_handling_options,
                              decode_up_to_bad_char);
    // We processed r.count characters of input.
    // We wrote temp_outlen bytes to temp_buffer.
    // If there is no ignorable characters,
    // we should expect that values/4.0*3 == temp_outlen,
    // except maybe at the tail end of the string.

    //
    // We are assuming that when r.error == error_code::OUTPUT_BUFFER_TOO_SMALL,
    // we truncate the results so that a number of base64 characters divisible
    // by four is processed.
    //

    //
    // We wrote temp_outlen bytes to temp_buffer.
    // We need to copy them to output.
    // Copy with relaxed atomic operations to the output
    simdutf_log_assert(temp_outlen <= outlen - actual_out,
                       "Output buffer is too small");
    simdutf_log_assert(temp_outlen <= temp_buffer.size(),
                       "Output buffer is too small");

    simdutf::scalar::memcpy_atomic_write(output + actual_out,
                                         temp_buffer.data(), temp_outlen);
    actual_out += temp_outlen;
    length -= r.count;
    input += r.count;

    if (r.error != error_code::OUTPUT_BUFFER_TOO_SMALL) {
      break;
    }
  }
  outlen = actual_out;
  return {r.error, size_t(input - input_init)};
}

simdutf_warn_unused result atomic_base64_to_binary_safe(
    const char *input, size_t length, char *output, size_t &outlen,
    base64_options options,
    last_chunk_handling_options last_chunk_handling_options,
    bool decode_up_to_bad_char) noexcept {
  return atomic_base64_to_binary_safe_impl<char>(
      input, length, output, outlen, options, last_chunk_handling_options,
      decode_up_to_bad_char);
}
simdutf_warn_unused result atomic_base64_to_binary_safe(
    const char16_t *input, size_t length, char *output, size_t &outlen,
    base64_options options,
    last_chunk_handling_options last_chunk_handling_options,
    bool decode_up_to_bad_char) noexcept {
  return atomic_base64_to_binary_safe_impl<char16_t>(
      input, length, output, outlen, options, last_chunk_handling_options,
      decode_up_to_bad_char);
}
  #endif // SIMDUTF_ATOMIC_REF

#endif // SIMDUTF_FEATURE_UTF16 || SIMDUTF_FEATURE_DETECT_ENCODING

#if SIMDUTF_FEATURE_UTF16
simdutf_warn_unused bool validate_utf16be(const char16_t *buf,
                                          size_t len) noexcept {
  return get_default_implementation()->validate_utf16be(buf, len);
}
simdutf_warn_unused result validate_utf16_with_errors(const char16_t *buf,
                                                      size_t len) noexcept {
  #if SIMDUTF_IS_BIG_ENDIAN
  return validate_utf16be_with_errors(buf, len);
  #else
  return validate_utf16le_with_errors(buf, len);
  #endif
}
simdutf_warn_unused result validate_utf16le_with_errors(const char16_t *buf,
                                                        size_t len) noexcept {
  return get_default_implementation()->validate_utf16le_with_errors(buf, len);
}
simdutf_warn_unused result validate_utf16be_with_errors(const char16_t *buf,
                                                        size_t len) noexcept {
  return get_default_implementation()->validate_utf16be_with_errors(buf, len);
}
#endif // SIMDUTF_FEATURE_UTF16

#if SIMDUTF_FEATURE_UTF32
simdutf_warn_unused bool validate_utf32(const char32_t *buf,
                                        size_t len) noexcept {
  return get_default_implementation()->validate_utf32(buf, len);
}
simdutf_warn_unused result validate_utf32_with_errors(const char32_t *buf,
                                                      size_t len) noexcept {
  return get_default_implementation()->validate_utf32_with_errors(buf, len);
}
#endif // SIMDUTF_FEATURE_UTF32

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16
simdutf_warn_unused size_t convert_valid_utf8_to_utf16(
    const char *input, size_t length, char16_t *utf16_buffer) noexcept {
  #if SIMDUTF_IS_BIG_ENDIAN
  return convert_valid_utf8_to_utf16be(input, length, utf16_buffer);
  #else
  return convert_valid_utf8_to_utf16le(input, length, utf16_buffer);
  #endif
}
simdutf_warn_unused size_t convert_valid_utf8_to_utf16le(
    const char *input, size_t length, char16_t *utf16_buffer) noexcept {
  return get_default_implementation()->convert_valid_utf8_to_utf16le(
      input, length, utf16_buffer);
}
simdutf_warn_unused size_t convert_valid_utf8_to_utf16be(
    const char *input, size_t length, char16_t *utf16_buffer) noexcept {
  return get_default_implementation()->convert_valid_utf8_to_utf16be(
      input, length, utf16_buffer);
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32
simdutf_warn_unused size_t convert_valid_utf8_to_utf32(
    const char *input, size_t length, char32_t *utf32_buffer) noexcept {
  return get_default_implementation()->convert_valid_utf8_to_utf32(
      input, length, utf32_buffer);
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16
simdutf_warn_unused size_t convert_utf16_to_utf8(const char16_t *buf,
                                                 size_t len,
                                                 char *utf8_buffer) noexcept {
  #if SIMDUTF_IS_BIG_ENDIAN
  return convert_utf16be_to_utf8(buf, len, utf8_buffer);
  #else
  return convert_utf16le_to_utf8(buf, len, utf8_buffer);
  #endif
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16

#if SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_LATIN1
simdutf_warn_unused size_t convert_utf16_to_latin1(
    const char16_t *buf, size_t len, char *latin1_buffer) noexcept {
  #if SIMDUTF_IS_BIG_ENDIAN
  return convert_utf16be_to_latin1(buf, len, latin1_buffer);
  #else
  return convert_utf16le_to_latin1(buf, len, latin1_buffer);
  #endif
}
simdutf_warn_unused size_t convert_latin1_to_utf16(
    const char *buf, size_t len, char16_t *utf16_output) noexcept {
  #if SIMDUTF_IS_BIG_ENDIAN
  return convert_latin1_to_utf16be(buf, len, utf16_output);
  #else
  return convert_latin1_to_utf16le(buf, len, utf16_output);
  #endif
}
simdutf_warn_unused size_t convert_utf16be_to_latin1(
    const char16_t *buf, size_t len, char *latin1_buffer) noexcept {
  return get_default_implementation()->convert_utf16be_to_latin1(buf, len,
                                                                 latin1_buffer);
}
simdutf_warn_unused size_t convert_utf16le_to_latin1(
    const char16_t *buf, size_t len, char *latin1_buffer) noexcept {
  return get_default_implementation()->convert_utf16le_to_latin1(buf, len,
                                                                 latin1_buffer);
}
simdutf_warn_unused size_t convert_valid_utf16be_to_latin1(
    const char16_t *buf, size_t len, char *latin1_buffer) noexcept {
  return get_default_implementation()->convert_valid_utf16be_to_latin1(
      buf, len, latin1_buffer);
}
simdutf_warn_unused size_t convert_valid_utf16le_to_latin1(
    const char16_t *buf, size_t len, char *latin1_buffer) noexcept {
  return get_default_implementation()->convert_valid_utf16le_to_latin1(
      buf, len, latin1_buffer);
}
simdutf_warn_unused result convert_utf16le_to_latin1_with_errors(
    const char16_t *buf, size_t len, char *latin1_buffer) noexcept {
  return get_default_implementation()->convert_utf16le_to_latin1_with_errors(
      buf, len, latin1_buffer);
}
simdutf_warn_unused result convert_utf16be_to_latin1_with_errors(
    const char16_t *buf, size_t len, char *latin1_buffer) noexcept {
  return get_default_implementation()->convert_utf16be_to_latin1_with_errors(
      buf, len, latin1_buffer);
}
simdutf_warn_unused size_t latin1_length_from_utf16(size_t length) noexcept {
  return length;
}
simdutf_warn_unused size_t utf16_length_from_latin1(size_t length) noexcept {
  return length;
}
#endif // SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_LATIN1

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16
simdutf_warn_unused size_t convert_utf16le_to_utf8(const char16_t *buf,
                                                   size_t len,
                                                   char *utf8_buffer) noexcept {
  return get_default_implementation()->convert_utf16le_to_utf8(buf, len,
                                                               utf8_buffer);
}
simdutf_warn_unused size_t convert_utf16be_to_utf8(const char16_t *buf,
                                                   size_t len,
                                                   char *utf8_buffer) noexcept {
  return get_default_implementation()->convert_utf16be_to_utf8(buf, len,
                                                               utf8_buffer);
}
simdutf_warn_unused result convert_utf16_to_utf8_with_errors(
    const char16_t *buf, size_t len, char *utf8_buffer) noexcept {
  #if SIMDUTF_IS_BIG_ENDIAN
  return convert_utf16be_to_utf8_with_errors(buf, len, utf8_buffer);
  #else
  return convert_utf16le_to_utf8_with_errors(buf, len, utf8_buffer);
  #endif
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16

#if SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_LATIN1
simdutf_warn_unused result convert_utf16_to_latin1_with_errors(
    const char16_t *buf, size_t len, char *latin1_buffer) noexcept {
  #if SIMDUTF_IS_BIG_ENDIAN
  return convert_utf16be_to_latin1_with_errors(buf, len, latin1_buffer);
  #else
  return convert_utf16le_to_latin1_with_errors(buf, len, latin1_buffer);
  #endif
}
#endif // SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_LATIN1

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16
simdutf_warn_unused result convert_utf16le_to_utf8_with_errors(
    const char16_t *buf, size_t len, char *utf8_buffer) noexcept {
  return get_default_implementation()->convert_utf16le_to_utf8_with_errors(
      buf, len, utf8_buffer);
}
simdutf_warn_unused result convert_utf16be_to_utf8_with_errors(
    const char16_t *buf, size_t len, char *utf8_buffer) noexcept {
  return get_default_implementation()->convert_utf16be_to_utf8_with_errors(
      buf, len, utf8_buffer);
}
simdutf_warn_unused size_t convert_valid_utf16_to_utf8(
    const char16_t *buf, size_t len, char *utf8_buffer) noexcept {
  #if SIMDUTF_IS_BIG_ENDIAN
  return convert_valid_utf16be_to_utf8(buf, len, utf8_buffer);
  #else
  return convert_valid_utf16le_to_utf8(buf, len, utf8_buffer);
  #endif
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16

#if SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_LATIN1
simdutf_warn_unused size_t convert_valid_utf16_to_latin1(
    const char16_t *buf, size_t len, char *latin1_buffer) noexcept {
  #if SIMDUTF_IS_BIG_ENDIAN
  return convert_valid_utf16be_to_latin1(buf, len, latin1_buffer);
  #else
  return convert_valid_utf16le_to_latin1(buf, len, latin1_buffer);
  #endif
}
#endif // SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_LATIN1

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16
simdutf_warn_unused size_t convert_valid_utf16le_to_utf8(
    const char16_t *buf, size_t len, char *utf8_buffer) noexcept {
  return get_default_implementation()->convert_valid_utf16le_to_utf8(
      buf, len, utf8_buffer);
}
simdutf_warn_unused size_t convert_valid_utf16be_to_utf8(
    const char16_t *buf, size_t len, char *utf8_buffer) noexcept {
  return get_default_implementation()->convert_valid_utf16be_to_utf8(
      buf, len, utf8_buffer);
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32
simdutf_warn_unused size_t convert_utf32_to_utf8(const char32_t *buf,
                                                 size_t len,
                                                 char *utf8_buffer) noexcept {
  return get_default_implementation()->convert_utf32_to_utf8(buf, len,
                                                             utf8_buffer);
}
simdutf_warn_unused result convert_utf32_to_utf8_with_errors(
    const char32_t *buf, size_t len, char *utf8_buffer) noexcept {
  return get_default_implementation()->convert_utf32_to_utf8_with_errors(
      buf, len, utf8_buffer);
}
simdutf_warn_unused size_t convert_valid_utf32_to_utf8(
    const char32_t *buf, size_t len, char *utf8_buffer) noexcept {
  return get_default_implementation()->convert_valid_utf32_to_utf8(buf, len,
                                                                   utf8_buffer);
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32

#if SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_UTF32
simdutf_warn_unused size_t convert_utf32_to_utf16(
    const char32_t *buf, size_t len, char16_t *utf16_buffer) noexcept {
  #if SIMDUTF_IS_BIG_ENDIAN
  return convert_utf32_to_utf16be(buf, len, utf16_buffer);
  #else
  return convert_utf32_to_utf16le(buf, len, utf16_buffer);
  #endif
}
#endif // SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_UTF32

#if SIMDUTF_FEATURE_UTF32 && SIMDUTF_FEATURE_LATIN1
simdutf_warn_unused size_t convert_utf32_to_latin1(
    const char32_t *input, size_t length, char *latin1_output) noexcept {
  return get_default_implementation()->convert_utf32_to_latin1(input, length,
                                                               latin1_output);
}
simdutf_warn_unused result convert_utf32_to_latin1_with_errors(
    const char32_t *input, size_t length, char *latin1_buffer) noexcept {
  return get_default_implementation()->convert_utf32_to_latin1_with_errors(
      input, length, latin1_buffer);
}
simdutf_warn_unused size_t convert_valid_utf32_to_latin1(
    const char32_t *input, size_t length, char *latin1_buffer) noexcept {
  return get_default_implementation()->convert_valid_utf32_to_latin1(
      input, length, latin1_buffer);
}
#endif // SIMDUTF_FEATURE_UTF32 && SIMDUTF_FEATURE_LATIN1

#if SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_UTF32
simdutf_warn_unused size_t convert_utf32_to_utf16le(
    const char32_t *buf, size_t len, char16_t *utf16_buffer) noexcept {
  return get_default_implementation()->convert_utf32_to_utf16le(buf, len,
                                                                utf16_buffer);
}
simdutf_warn_unused size_t convert_utf32_to_utf16be(
    const char32_t *buf, size_t len, char16_t *utf16_buffer) noexcept {
  return get_default_implementation()->convert_utf32_to_utf16be(buf, len,
                                                                utf16_buffer);
}
simdutf_warn_unused result convert_utf32_to_utf16_with_errors(
    const char32_t *buf, size_t len, char16_t *utf16_buffer) noexcept {
  #if SIMDUTF_IS_BIG_ENDIAN
  return convert_utf32_to_utf16be_with_errors(buf, len, utf16_buffer);
  #else
  return convert_utf32_to_utf16le_with_errors(buf, len, utf16_buffer);
  #endif
}
simdutf_warn_unused result convert_utf32_to_utf16le_with_errors(
    const char32_t *buf, size_t len, char16_t *utf16_buffer) noexcept {
  return get_default_implementation()->convert_utf32_to_utf16le_with_errors(
      buf, len, utf16_buffer);
}
simdutf_warn_unused result convert_utf32_to_utf16be_with_errors(
    const char32_t *buf, size_t len, char16_t *utf16_buffer) noexcept {
  return get_default_implementation()->convert_utf32_to_utf16be_with_errors(
      buf, len, utf16_buffer);
}
simdutf_warn_unused size_t convert_valid_utf32_to_utf16(
    const char32_t *buf, size_t len, char16_t *utf16_buffer) noexcept {
  #if SIMDUTF_IS_BIG_ENDIAN
  return convert_valid_utf32_to_utf16be(buf, len, utf16_buffer);
  #else
  return convert_valid_utf32_to_utf16le(buf, len, utf16_buffer);
  #endif
}
simdutf_warn_unused size_t convert_valid_utf32_to_utf16le(
    const char32_t *buf, size_t len, char16_t *utf16_buffer) noexcept {
  return get_default_implementation()->convert_valid_utf32_to_utf16le(
      buf, len, utf16_buffer);
}
simdutf_warn_unused size_t convert_valid_utf32_to_utf16be(
    const char32_t *buf, size_t len, char16_t *utf16_buffer) noexcept {
  return get_default_implementation()->convert_valid_utf32_to_utf16be(
      buf, len, utf16_buffer);
}
simdutf_warn_unused size_t convert_utf16_to_utf32(
    const char16_t *buf, size_t len, char32_t *utf32_buffer) noexcept {
  #if SIMDUTF_IS_BIG_ENDIAN
  return convert_utf16be_to_utf32(buf, len, utf32_buffer);
  #else
  return convert_utf16le_to_utf32(buf, len, utf32_buffer);
  #endif
}
simdutf_warn_unused size_t convert_utf16le_to_utf32(
    const char16_t *buf, size_t len, char32_t *utf32_buffer) noexcept {
  return get_default_implementation()->convert_utf16le_to_utf32(buf, len,
                                                                utf32_buffer);
}
simdutf_warn_unused size_t convert_utf16be_to_utf32(
    const char16_t *buf, size_t len, char32_t *utf32_buffer) noexcept {
  return get_default_implementation()->convert_utf16be_to_utf32(buf, len,
                                                                utf32_buffer);
}
simdutf_warn_unused result convert_utf16_to_utf32_with_errors(
    const char16_t *buf, size_t len, char32_t *utf32_buffer) noexcept {
  #if SIMDUTF_IS_BIG_ENDIAN
  return convert_utf16be_to_utf32_with_errors(buf, len, utf32_buffer);
  #else
  return convert_utf16le_to_utf32_with_errors(buf, len, utf32_buffer);
  #endif
}
simdutf_warn_unused result convert_utf16le_to_utf32_with_errors(
    const char16_t *buf, size_t len, char32_t *utf32_buffer) noexcept {
  return get_default_implementation()->convert_utf16le_to_utf32_with_errors(
      buf, len, utf32_buffer);
}
simdutf_warn_unused result convert_utf16be_to_utf32_with_errors(
    const char16_t *buf, size_t len, char32_t *utf32_buffer) noexcept {
  return get_default_implementation()->convert_utf16be_to_utf32_with_errors(
      buf, len, utf32_buffer);
}
simdutf_warn_unused size_t convert_valid_utf16_to_utf32(
    const char16_t *buf, size_t len, char32_t *utf32_buffer) noexcept {
  #if SIMDUTF_IS_BIG_ENDIAN
  return convert_valid_utf16be_to_utf32(buf, len, utf32_buffer);
  #else
  return convert_valid_utf16le_to_utf32(buf, len, utf32_buffer);
  #endif
}
simdutf_warn_unused size_t convert_valid_utf16le_to_utf32(
    const char16_t *buf, size_t len, char32_t *utf32_buffer) noexcept {
  return get_default_implementation()->convert_valid_utf16le_to_utf32(
      buf, len, utf32_buffer);
}
simdutf_warn_unused size_t convert_valid_utf16be_to_utf32(
    const char16_t *buf, size_t len, char32_t *utf32_buffer) noexcept {
  return get_default_implementation()->convert_valid_utf16be_to_utf32(
      buf, len, utf32_buffer);
}
#endif // SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_UTF32

#if SIMDUTF_FEATURE_UTF16
void change_endianness_utf16(const char16_t *input, size_t length,
                             char16_t *output) noexcept {
  get_default_implementation()->change_endianness_utf16(input, length, output);
}
simdutf_warn_unused size_t count_utf16(const char16_t *input,
                                       size_t length) noexcept {
  #if SIMDUTF_IS_BIG_ENDIAN
  return count_utf16be(input, length);
  #else
  return count_utf16le(input, length);
  #endif
}
simdutf_warn_unused size_t count_utf16le(const char16_t *input,
                                         size_t length) noexcept {
  return get_default_implementation()->count_utf16le(input, length);
}
simdutf_warn_unused size_t count_utf16be(const char16_t *input,
                                         size_t length) noexcept {
  return get_default_implementation()->count_utf16be(input, length);
}
#endif // SIMDUTF_FEATURE_UTF16

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1
simdutf_warn_unused size_t count_utf8(const char *input,
                                      size_t length) noexcept {
  return get_default_implementation()->count_utf8(input, length);
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1
simdutf_warn_unused size_t latin1_length_from_utf8(const char *buf,
                                                   size_t len) noexcept {
  return get_default_implementation()->latin1_length_from_utf8(buf, len);
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1
simdutf_warn_unused size_t utf8_length_from_latin1(const char *buf,
                                                   size_t len) noexcept {
  return get_default_implementation()->utf8_length_from_latin1(buf, len);
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16
simdutf_warn_unused size_t utf8_length_from_utf16(const char16_t *input,
                                                  size_t length) noexcept {
  #if SIMDUTF_IS_BIG_ENDIAN
  return utf8_length_from_utf16be(input, length);
  #else
  return utf8_length_from_utf16le(input, length);
  #endif
}
simdutf_warn_unused size_t utf8_length_from_utf16le(const char16_t *input,
                                                    size_t length) noexcept {
  return get_default_implementation()->utf8_length_from_utf16le(input, length);
}
simdutf_warn_unused size_t utf8_length_from_utf16be(const char16_t *input,
                                                    size_t length) noexcept {
  return get_default_implementation()->utf8_length_from_utf16be(input, length);
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16

#if SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_UTF32
simdutf_warn_unused size_t utf32_length_from_utf16(const char16_t *input,
                                                   size_t length) noexcept {
  #if SIMDUTF_IS_BIG_ENDIAN
  return utf32_length_from_utf16be(input, length);
  #else
  return utf32_length_from_utf16le(input, length);
  #endif
}
simdutf_warn_unused size_t utf32_length_from_utf16le(const char16_t *input,
                                                     size_t length) noexcept {
  return get_default_implementation()->utf32_length_from_utf16le(input, length);
}
simdutf_warn_unused size_t utf32_length_from_utf16be(const char16_t *input,
                                                     size_t length) noexcept {
  return get_default_implementation()->utf32_length_from_utf16be(input, length);
}
#endif // SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_UTF32

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16
simdutf_warn_unused size_t utf16_length_from_utf8(const char *input,
                                                  size_t length) noexcept {
  return get_default_implementation()->utf16_length_from_utf8(input, length);
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32
simdutf_warn_unused size_t utf8_length_from_utf32(const char32_t *input,
                                                  size_t length) noexcept {
  return get_default_implementation()->utf8_length_from_utf32(input, length);
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32

#if SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_UTF32
simdutf_warn_unused size_t utf16_length_from_utf32(const char32_t *input,
                                                   size_t length) noexcept {
  return get_default_implementation()->utf16_length_from_utf32(input, length);
}
#endif // SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_UTF32

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32
simdutf_warn_unused size_t utf32_length_from_utf8(const char *input,
                                                  size_t length) noexcept {
  return get_default_implementation()->utf32_length_from_utf8(input, length);
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF32

#if SIMDUTF_FEATURE_BASE64
simdutf_warn_unused size_t
maximal_binary_length_from_base64(const char *input, size_t length) noexcept {
  return get_default_implementation()->maximal_binary_length_from_base64(
      input, length);
}

simdutf_warn_unused result base64_to_binary(
    const char *input, size_t length, char *output, base64_options options,
    last_chunk_handling_options last_chunk_handling_options) noexcept {
  return get_default_implementation()->base64_to_binary(
      input, length, output, options, last_chunk_handling_options);
}

simdutf_warn_unused size_t maximal_binary_length_from_base64(
    const char16_t *input, size_t length) noexcept {
  return get_default_implementation()->maximal_binary_length_from_base64(
      input, length);
}

simdutf_warn_unused result base64_to_binary(
    const char16_t *input, size_t length, char *output, base64_options options,
    last_chunk_handling_options last_chunk_handling_options) noexcept {
  return get_default_implementation()->base64_to_binary(
      input, length, output, options, last_chunk_handling_options);
}

template <typename chartype>
simdutf_warn_unused result slow_base64_to_binary_safe_impl(
    const chartype *input, size_t length, char *output, size_t &outlen,
    base64_options options,
    last_chunk_handling_options last_chunk_options) noexcept {
  const bool ignore_garbage = (options & base64_default_accept_garbage) != 0;
  auto ri = simdutf::scalar::base64::find_end(input, length, options);
  size_t equallocation = ri.equallocation;
  size_t equalsigns = ri.equalsigns;
  length = ri.srclen;
  size_t full_input_length = ri.full_input_length;
  (void)full_input_length;
  if (length == 0) {
    outlen = 0;
    if (!ignore_garbage && equalsigns > 0) {
      return {INVALID_BASE64_CHARACTER, equallocation};
    }
    return {SUCCESS, 0};
  }

  // The parameters of base64_tail_decode_safe are:
  // - dst: the output buffer
  // - outlen: the size of the output buffer
  // - srcr: the input buffer
  // - length: the size of the input buffer
  // - padded_characters: the number of padding characters
  // - options: the options for the base64 decoder
  // - last_chunk_options: the options for the last chunk
  // The function will return the number of bytes written to the output buffer
  // and the number of bytes read from the input buffer.
  // The function will also return an error code if the input buffer is not
  // valid base64.
  full_result r = scalar::base64::base64_tail_decode_safe(
      output, outlen, input, length, equalsigns, options, last_chunk_options);
  r = scalar::base64::patch_tail_result(r, 0, 0, equallocation,
                                        full_input_length, last_chunk_options);
  outlen = r.output_count;
  if (!is_partial(last_chunk_options) && r.error == error_code::SUCCESS &&
      equalsigns > 0) {
    // additional checks
    if ((outlen % 3 == 0) || ((outlen % 3) + 1 + equalsigns != 4)) {
      r.error = error_code::INVALID_BASE64_CHARACTER;
    }
  }
  return {r.error, r.input_count}; // we cannot return r itself because it gets
                                   // converted to error/output_count
}
simdutf_warn_unused bool base64_ignorable(char input,
                                          base64_options options) noexcept {
  return scalar::base64::is_ignorable(input, options);
}
simdutf_warn_unused bool base64_ignorable(char16_t input,
                                          base64_options options) noexcept {
  return scalar::base64::is_ignorable(input, options);
}
simdutf_warn_unused bool base64_valid(char input,
                                      base64_options options) noexcept {
  return scalar::base64::is_base64(input, options);
}
simdutf_warn_unused bool base64_valid(char16_t input,
                                      base64_options options) noexcept {
  return scalar::base64::is_base64(input, options);
}
simdutf_warn_unused bool
base64_valid_or_padding(char input, base64_options options) noexcept {
  return scalar::base64::is_base64_or_padding(input, options);
}
simdutf_warn_unused bool
base64_valid_or_padding(char16_t input, base64_options options) noexcept {
  return scalar::base64::is_base64_or_padding(input, options);
}

template <typename chartype>
simdutf_warn_unused result base64_to_binary_safe_impl(
    const chartype *input, size_t length, char *output, size_t &outlen,
    base64_options options,
    last_chunk_handling_options last_chunk_handling_options,
    bool decode_up_to_bad_char) noexcept {
  static_assert(std::is_same<chartype, char>::value ||
                    std::is_same<chartype, char16_t>::value,
                "Only char and char16_t are supported.");
  size_t remaining_input_length = length;
  size_t remaining_output_length = outlen;
  size_t input_position = 0;
  size_t output_position = 0;

  // We also do a first pass using the fast path to decode as much as possible
  size_t safe_input = (std::min)(
      remaining_input_length,
      base64_length_from_binary(remaining_output_length / 3 * 3, options));
  bool done_with_partial = (safe_input == remaining_input_length);
  simdutf::full_result r =
      get_default_implementation()->base64_to_binary_details(
          input + input_position, safe_input, output + output_position, options,
          done_with_partial
              ? last_chunk_handling_options
              : simdutf::last_chunk_handling_options::only_full_chunks);
  simdutf_log_assert(r.input_count <= safe_input,
                     "You should not read more than safe_input");
  simdutf_log_assert(r.output_count <= remaining_output_length,
                     "You should not write more than remaining_output_length");
  // Technically redundant, but we want to be explicit about it.
  input_position += r.input_count;
  output_position += r.output_count;
  remaining_input_length -= r.input_count;
  remaining_output_length -= r.output_count;
  if (r.error != simdutf::error_code::SUCCESS) {
    // There is an error. We return.
    if (decode_up_to_bad_char &&
        r.error == error_code::INVALID_BASE64_CHARACTER) {
      return slow_base64_to_binary_safe_impl(
          input, length, output, outlen, options, last_chunk_handling_options);
    }
    outlen = output_position;
    return {r.error, input_position};
  }
  if (done_with_partial) {
    // We are done. We have decoded everything.
    outlen = output_position;
    return {simdutf::error_code::SUCCESS, input_position};
  }

  // We have decoded some data, but we still have some data to decode.
  // We need to decode the rest of the input buffer.
  r = simdutf::scalar::base64::base64_to_binary_details_safe_impl(
      input + input_position, remaining_input_length, output + output_position,
      remaining_output_length, options, last_chunk_handling_options);

  input_position += r.input_count;
  output_position += r.output_count;
  remaining_input_length -= r.input_count;
  remaining_output_length -= r.output_count;

  if (r.error != simdutf::error_code::SUCCESS) {
    // There is an error. We return.
    if (decode_up_to_bad_char &&
        r.error == error_code::INVALID_BASE64_CHARACTER) {
      return slow_base64_to_binary_safe_impl(
          input, length, output, outlen, options, last_chunk_handling_options);
    }
    outlen = output_position;
    return {r.error, input_position};
  }
  outlen = output_position;
  return {simdutf::error_code::SUCCESS, input_position};
}

  #if SIMDUTF_ATOMIC_REF
size_t atomic_binary_to_base64(const char *input, size_t length, char *output,
                               base64_options options) noexcept {
  size_t retval = 0;
    #if defined(FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION)
  // We use a smaller buffer during fuzzing to more easily detect bugs.
  constexpr size_t input_block_size = 128 * 3;
    #else
  // Arbitrary block sizes: 3KB for input which produces 4KB in output.
  constexpr size_t input_block_size = 1024 * 3;
    #endif
  std::array<char, input_block_size> inbuf;
  for (size_t i = 0; i < length; i += input_block_size) {
    const size_t current_block_size = std::min(input_block_size, length - i);
    simdutf::scalar::memcpy_atomic_read(inbuf.data(), input + i,
                                        current_block_size);
    const size_t written = binary_to_base64(inbuf.data(), current_block_size,
                                            output + retval, options);
    retval += written;
  }
  return retval;
}
  #endif // SIMDUTF_ATOMIC_REF

#endif // SIMDUTF_FEATURE_BASE64

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1
simdutf_warn_unused size_t convert_latin1_to_utf8_safe(
    const char *buf, size_t len, char *utf8_output, size_t utf8_len) noexcept {
  const auto start{utf8_output};

  while (true) {
    // convert_latin1_to_utf8 will never write more than input length * 2
    auto read_len = std::min(len, utf8_len >> 1);
    if (read_len <= 16) {
      break;
    }

    const auto write_len =
        simdutf::convert_latin1_to_utf8(buf, read_len, utf8_output);

    utf8_output += write_len;
    utf8_len -= write_len;
    buf += read_len;
    len -= read_len;
  }

  utf8_output +=
      scalar::latin1_to_utf8::convert_safe(buf, len, utf8_output, utf8_len);

  return utf8_output - start;
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_LATIN1

#if SIMDUTF_FEATURE_BASE64
simdutf_warn_unused result
base64_to_binary_safe(const char *input, size_t length, char *output,
                      size_t &outlen, base64_options options,
                      last_chunk_handling_options last_chunk_handling_options,
                      bool decode_up_to_bad_char) noexcept {
  return base64_to_binary_safe_impl<char>(input, length, output, outlen,
                                          options, last_chunk_handling_options,
                                          decode_up_to_bad_char);
}
simdutf_warn_unused result
base64_to_binary_safe(const char16_t *input, size_t length, char *output,
                      size_t &outlen, base64_options options,
                      last_chunk_handling_options last_chunk_handling_options,
                      bool decode_up_to_bad_char) noexcept {
  return base64_to_binary_safe_impl<char16_t>(
      input, length, output, outlen, options, last_chunk_handling_options,
      decode_up_to_bad_char);
}

simdutf_warn_unused size_t
base64_length_from_binary(size_t length, base64_options options) noexcept {
  return get_default_implementation()->base64_length_from_binary(length,
                                                                 options);
}

size_t binary_to_base64(const char *input, size_t length, char *output,
                        base64_options options) noexcept {
  return get_default_implementation()->binary_to_base64(input, length, output,
                                                        options);
}
#endif // SIMDUTF_FEATURE_BASE64

#if SIMDUTF_FEATURE_DETECT_ENCODING
simdutf_warn_unused simdutf::encoding_type
autodetect_encoding(const char *buf, size_t length) noexcept {
  return get_default_implementation()->autodetect_encoding(buf, length);
}

simdutf_warn_unused int detect_encodings(const char *buf,
                                         size_t length) noexcept {
  return get_default_implementation()->detect_encodings(buf, length);
}
#endif // SIMDUTF_FEATURE_DETECT_ENCODING

const implementation *builtin_implementation() {
  static const implementation *builtin_impl =
      get_available_implementations()[SIMDUTF_STRINGIFY(
          SIMDUTF_BUILTIN_IMPLEMENTATION)];
  return builtin_impl;
}

#if SIMDUTF_FEATURE_UTF8
simdutf_warn_unused size_t trim_partial_utf8(const char *input, size_t length) {
  return scalar::utf8::trim_partial_utf8(input, length);
}
#endif // SIMDUTF_FEATURE_UTF8

#if SIMDUTF_FEATURE_UTF16
simdutf_warn_unused size_t trim_partial_utf16be(const char16_t *input,
                                                size_t length) {
  return scalar::utf16::trim_partial_utf16<BIG>(input, length);
}

simdutf_warn_unused size_t trim_partial_utf16le(const char16_t *input,
                                                size_t length) {
  return scalar::utf16::trim_partial_utf16<LITTLE>(input, length);
}

simdutf_warn_unused size_t trim_partial_utf16(const char16_t *input,
                                              size_t length) {
  #if SIMDUTF_IS_BIG_ENDIAN
  return trim_partial_utf16be(input, length);
  #else
  return trim_partial_utf16le(input, length);
  #endif
}
#endif // SIMDUTF_FEATURE_UTF16

} // namespace simdutf
