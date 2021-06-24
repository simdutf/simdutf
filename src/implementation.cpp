#include "simdutf.h"
#include <initializer_list>

namespace simdutf {
bool implementation::supported_by_runtime_system() const {
  uint32_t required_instruction_sets = this->required_instruction_sets();
  uint32_t supported_instruction_sets = internal::detect_supported_architectures();
  return ((supported_instruction_sets & required_instruction_sets) == required_instruction_sets);
}

simdutf_warn_unused encoding_type implementation::autodetect_encoding(const char * input, size_t length) const noexcept {
    // If there is a BOM, then we trust it.
    auto bom_encoding = simdutf::BOM::check_bom(input, length);
    if(bom_encoding != encoding_type::unspecified) { return bom_encoding; }
    // UTF8 is common, it includes ASCII, and is commonly represented
    // without a BOM, so if it fits, go with that. Note that it is still
    // possible to get it wrong, we are only 'guessing'. If some has UTF-16
    // data without a BOM, it could pass as UTF-8.
    //
    // An interesting twist might be to check for UTF-16 ASCII first (every
    // other byte is zero).
    if(validate_utf8(input, length)) { return encoding_type::UTF8; }
    // The next most common encoding that might appear without BOM is probably
    // UTF-16LE, so try that next.
    if((length % 2) == 0) {
      if(validate_utf16(reinterpret_cast<const char16_t*>(input), length)) { return encoding_type::UTF16_LE; }
    }
    return encoding_type::unspecified;
}

namespace internal {

// Static array of known implementations. We're hoping these get baked into the executable
// without requiring a static initializer.

#if SIMDUTF_IMPLEMENTATION_HASWELL
const haswell::implementation haswell_singleton{};
#endif
#if SIMDUTF_IMPLEMENTATION_WESTMERE
const westmere::implementation westmere_singleton{};
#endif // SIMDUTF_IMPLEMENTATION_WESTMERE
#if SIMDUTF_IMPLEMENTATION_ARM64
const arm64::implementation arm64_singleton{};
#endif // SIMDUTF_IMPLEMENTATION_ARM64
#if SIMDUTF_IMPLEMENTATION_PPC64
const ppc64::implementation ppc64_singleton{};
#endif // SIMDUTF_IMPLEMENTATION_PPC64
#if SIMDUTF_IMPLEMENTATION_FALLBACK
const fallback::implementation fallback_singleton{};
#endif // SIMDUTF_IMPLEMENTATION_FALLBACK

/**
 * @private Detects best supported implementation on first use, and sets it
 */
class detect_best_supported_implementation_on_first_use final : public implementation {
public:
  const std::string &name() const noexcept final { return set_best()->name(); }
  const std::string &description() const noexcept final { return set_best()->description(); }
  uint32_t required_instruction_sets() const noexcept final { return set_best()->required_instruction_sets(); }

  simdutf_warn_unused bool validate_utf8(const char * buf, size_t len) const noexcept final override {
    return set_best()->validate_utf8(buf, len);
  }

  simdutf_warn_unused bool validate_utf16(const char16_t * buf, size_t len) const noexcept final override {
    return set_best()->validate_utf16(buf, len);
  }

  simdutf_warn_unused size_t convert_utf8_to_utf16(const char * buf, size_t len, char16_t* utf16_output) const noexcept final override {
    return set_best()->convert_utf8_to_utf16(buf, len, utf16_output);
  }

  simdutf_warn_unused size_t convert_valid_utf8_to_utf16(const char * buf, size_t len, char16_t* utf16_output) const noexcept final override {
    return set_best()->convert_valid_utf8_to_utf16(buf, len, utf16_output);
  }

  simdutf_warn_unused size_t convert_utf16_to_utf8(const char16_t * buf, size_t len, char* utf8_output) const noexcept final override {
    return set_best()->convert_utf16_to_utf8(buf, len, utf8_output);
  }

  simdutf_warn_unused size_t convert_valid_utf16_to_utf8(const char16_t * buf, size_t len, char* utf8_output) const noexcept final override {
    return set_best()->convert_valid_utf16_to_utf8(buf, len, utf8_output);
  }

  simdutf_warn_unused size_t count_utf16(const char16_t * buf, size_t len) const noexcept final override {
    return set_best()->count_utf16(buf, len);
  }

  simdutf_warn_unused size_t count_utf8(const char * buf, size_t len) const noexcept final override {
    return set_best()->count_utf8(buf, len);
  }

  simdutf_really_inline detect_best_supported_implementation_on_first_use() noexcept : implementation("best_supported_detector", "Detects the best supported implementation and sets it", 0) {}

private:
  const implementation *set_best() const noexcept;
};

const detect_best_supported_implementation_on_first_use detect_best_supported_implementation_on_first_use_singleton;

const std::initializer_list<const implementation *> available_implementation_pointers {
#if SIMDUTF_IMPLEMENTATION_HASWELL
  &haswell_singleton,
#endif
#if SIMDUTF_IMPLEMENTATION_WESTMERE
  &westmere_singleton,
#endif
#if SIMDUTF_IMPLEMENTATION_ARM64
  &arm64_singleton,
#endif
#if SIMDUTF_IMPLEMENTATION_PPC64
  &ppc64_singleton,
#endif
#if SIMDUTF_IMPLEMENTATION_FALLBACK
  &fallback_singleton,
#endif
}; // available_implementation_pointers

// So we can return UNSUPPORTED_ARCHITECTURE from the parser when there is no support
class unsupported_implementation final : public implementation {
public:
  simdutf_warn_unused bool validate_utf8(const char *, size_t) const noexcept final override {
    return false; // Just refuse to validate. Given that we have a fallback implementation
    // it seems unlikely that unsupported_implementation will ever be used. If it is used,
    // then it will flag all strings as invalid. The alternative is to return an error_code
    // from which the user has to figure out whether the string is valid UTF-8... which seems
    // like a lot of work just to handle the very unlikely case that we have an unsupported
    // implementation. And, when it does happen (that we have an unsupported implementation),
    // what are the chances that the programmer has a fallback? Given that *we* provide the
    // fallback, it implies that the programmer would need a fallback for our fallback.
  }

  simdutf_warn_unused bool validate_utf16(const char16_t*, size_t) const noexcept final override {
    return false;
  }

  simdutf_warn_unused size_t convert_utf8_to_utf16(const char*, size_t, char16_t*) const noexcept final override {
    return 0;
  }

  simdutf_warn_unused size_t convert_valid_utf8_to_utf16(const char*, size_t, char16_t*) const noexcept final override {
    return 0;
  }

  simdutf_warn_unused size_t convert_utf16_to_utf8(const char16_t*, size_t, char*) const noexcept final override {
    return 0;
  }

  simdutf_warn_unused size_t convert_valid_utf16_to_utf8(const char16_t*, size_t, char*) const noexcept final override {
    return 0;
  }

  simdutf_warn_unused size_t count_utf16(const char16_t *, size_t) const noexcept final override {
    return 0;
  }

  simdutf_warn_unused size_t count_utf8(const char *, size_t) const noexcept final override {
    return 0;
  }

  unsupported_implementation() : implementation("unsupported", "Unsupported CPU (no detected SIMD instructions)", 0) {}
};

const unsupported_implementation unsupported_singleton{};

size_t available_implementation_list::size() const noexcept {
  return internal::available_implementation_pointers.size();
}
const implementation * const *available_implementation_list::begin() const noexcept {
  return internal::available_implementation_pointers.begin();
}
const implementation * const *available_implementation_list::end() const noexcept {
  return internal::available_implementation_pointers.end();
}
const implementation *available_implementation_list::detect_best_supported() const noexcept {
  // They are prelisted in priority order, so we just go down the list
  uint32_t supported_instruction_sets = internal::detect_supported_architectures();
  for (const implementation *impl : internal::available_implementation_pointers) {
    uint32_t required_instruction_sets = impl->required_instruction_sets();
    if ((supported_instruction_sets & required_instruction_sets) == required_instruction_sets) { return impl; }
  }
  return &unsupported_singleton; // this should never happen?
}

const implementation *detect_best_supported_implementation_on_first_use::set_best() const noexcept {
  SIMDUTF_PUSH_DISABLE_WARNINGS
  SIMDUTF_DISABLE_DEPRECATED_WARNING // Disable CRT_SECURE warning on MSVC: manually verified this is safe
  char *force_implementation_name = getenv("SIMDUTF_FORCE_IMPLEMENTATION");
  SIMDUTF_POP_DISABLE_WARNINGS

  if (force_implementation_name) {
    auto force_implementation = available_implementations[force_implementation_name];
    if (force_implementation) {
      return active_implementation = force_implementation;
    } else {
      // Note: abort() and stderr usage within the library is forbidden.
      return active_implementation = &unsupported_singleton;
    }
  }
  return active_implementation = available_implementations.detect_best_supported();
}

} // namespace internal

SIMDUTF_DLLIMPORTEXPORT const internal::available_implementation_list available_implementations{};
SIMDUTF_DLLIMPORTEXPORT internal::atomic_ptr<const implementation> active_implementation{&internal::detect_best_supported_implementation_on_first_use_singleton};

simdutf_warn_unused bool validate_utf8(const char *buf, size_t len) noexcept {
  return active_implementation->validate_utf8(buf, len);
}
simdutf_warn_unused size_t convert_utf8_to_utf16(const char * input, size_t length, char16_t* utf16_output) noexcept {
  return active_implementation->convert_utf8_to_utf16(input, length, utf16_output);
}
simdutf_warn_unused bool validate_utf16(const char16_t * buf, size_t len) noexcept {
  return active_implementation->validate_utf16(buf, len);
}
simdutf_warn_unused size_t convert_valid_utf8_to_utf16(const char * input, size_t length, char16_t* utf16_buffer) noexcept {
  return active_implementation->convert_valid_utf8_to_utf16(input, length, utf16_buffer);
}
simdutf_warn_unused size_t convert_utf16_to_utf8(const char16_t * buf, size_t len, char* utf8_buffer) noexcept {
  return active_implementation->convert_utf16_to_utf8(buf, len, utf8_buffer);
}
simdutf_warn_unused size_t convert_valid_utf16_to_utf8(const char16_t * buf, size_t len, char* utf8_buffer) noexcept {
  return active_implementation->convert_valid_utf16_to_utf8(buf, len, utf8_buffer);
}
simdutf_warn_unused size_t count_utf16(const char16_t * input, size_t length) noexcept {
  return active_implementation->count_utf16(input, length);
}
simdutf_warn_unused size_t count_utf8(const char * input, size_t length) noexcept {
  return active_implementation->count_utf8(input, length);
} 
simdutf_warn_unused simdutf::encoding_type autodetect_encoding(const char * buf, size_t length) noexcept {
  return active_implementation->autodetect_encoding(buf, length);
}

const implementation * builtin_implementation() {
  static const implementation * builtin_impl = available_implementations[STRINGIFY(SIMDUTF_BUILTIN_IMPLEMENTATION)];
  return builtin_impl;
}


} // namespace simdutf
