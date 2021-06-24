#ifndef SIMDUTF_IMPLEMENTATION_H
#define SIMDUTF_IMPLEMENTATION_H
#include <string>
#include <atomic>
#include <vector>
#include "simdutf/common_defs.h"
#include "simdutf/internal/isadetection.h"


namespace simdutf {

/**
 * Autodetect the encoding of the input.
 *
 * @param input the string to analyze.
 * @param length the length of the string in bytes.
 * @return the detected encoding type
 */
simdutf_warn_unused simdutf::encoding_type autodetect_encoding(const char * input, size_t length) noexcept;
simdutf_really_inline simdutf_warn_unused simdutf::encoding_type autodetect_encoding(const uint8_t * input, size_t length) noexcept {
  return autodetect_encoding(reinterpret_cast<const char *>(input), length);
}
simdutf_really_inline simdutf_warn_unused simdutf::encoding_type autodetect_encoding(const std::string_view sv) noexcept {
  return autodetect_encoding(sv.data(), sv.size());
}


/**
 * Validate the UTF-8 string.
 *
 * @param input the string to validate.
 * @param length the length of the string in bytes.
 * @return true if the string is valid UTF-8.
 */
simdutf_warn_unused bool validate_utf8(const char * input, size_t length) noexcept;


/**
 * Validate the UTF-8 string.
 *
 * @param sv the string_view to validate.
 * @return true if the string is valid UTF-8.
 */
simdutf_really_inline simdutf_warn_unused bool validate_utf8(const std::string_view sv) noexcept {
  return validate_utf8(sv.data(), sv.size());
}

/**
 * Validate the UTF-16 string.
 *
 * @param buf the string to validate.
 * @param len the length of the string in bytes.
 * @return true if the string is valid UTF-16.
 */
simdutf_warn_unused bool validate_utf16(const char16_t * buf, size_t len) noexcept;

/**
 * Validate the UTF-8 string.
 *
 * @param p the string to validate.
 * @return true if the string is valid UTF-8.
 */
simdutf_really_inline simdutf_warn_unused bool validate_utf8(const std::string& s) noexcept {
  return validate_utf8(s.data(), s.size());
}

/**
 * Validate the UTF-16 string.
 *
 * @param sv the string_view to validate.
 * @return true if the string is valid UTF-16.
 */
simdutf_really_inline simdutf_warn_unused bool validate_utf16(const std::string_view sv) noexcept {
  return validate_utf16(reinterpret_cast<const char16_t*>(sv.data()), sv.size());
}

/**
 * Validate the UTF-16 string.
 *
 * @param s the string to validate.
 * @return true if the string is valid UTF-16.
 */
simdutf_really_inline simdutf_warn_unused bool validate_utf16(const std::string& s) noexcept {
  return validate_utf16(reinterpret_cast<const char16_t*>(s.data()), s.size());
}

/**
 * Convert possibly broken UTF-8 string into UTF-16 string.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * @param input         the string to convert
 * @param length        the length of the string in bytes
 * @param utf16_buffer  the pointer to buffer that can hold conversion result
 * @return the number of written bytes; 0 if the input was not valid UTF-8 string
 */
simdutf_warn_unused size_t convert_utf8_to_utf16(const char * input, size_t length, char16_t* utf16_output) noexcept;

/**
 * Convert valid UTF-8 string into UTF-16 string.
 *
 * This function assumes that the input string is valid UTF-8.
 *
 * @param input         the string to convert
 * @param length        the length of the string in bytes
 * @param utf16_buffer  the pointer to buffer that can hold conversion result
 * @return the number of written char16_t words
 */
simdutf_warn_unused size_t convert_valid_utf8_to_utf16(const char * input, size_t length, char16_t* utf16_buffer) noexcept;

/**
 * Convert possibly broken UTF-16 string into UTF-8 string.
 *
 * During the conversion also validation of the input string is done.
 * This function is suitable to work with inputs from untrusted sources.
 *
 * @param input         the string to convert
 * @param length        the length of the string in bytes
 * @param utf8_buffer   the pointer to buffer that can hold conversion result
 * @return number of written char16_t words; 0 if input is not a valid UTF-16 string
 */
simdutf_warn_unused size_t convert_utf16_to_utf8(const char16_t * buf, size_t len, char* utf8_buffer) noexcept;

/**
 * Convert valid UTF-16 string into UTF-8 string.
 *
 * This function assumes that the input string is valid UTF-16.
 *
 * @param input         the string to convert
 * @param length        the length of the string in bytes
 * @param utf8_buffer   the pointer to buffer that can hold conversion result
 * @return number of written bytes; 0 if conversion is not possible
 */
simdutf_warn_unused size_t convert_valid_utf16_to_utf8(const char16_t * buf, size_t len, char* utf8_buffer) noexcept;


/**
 * Count the number of code points (characters) in the string assuming that
 * it is valid.
 *
 * This function assumes that the input string is valid UTF-16.
 *
 * This function is not BOM-aware.
 *
 * @param input         the string to process
 * @param length        the length of the string in words
 * @return number of code points
 */
simdutf_warn_unused size_t count_utf16(const char16_t * input, size_t length) noexcept;

/**
 * Count the number of code points (characters) in the string assuming that
 * it is valid.
 *
 * This function assumes that the input string is valid UTF-8.
 *
 * @param input         the string to process
 * @param length        the length of the string in bytes
 * @return number of code points
 */
simdutf_warn_unused size_t count_utf8(const char * input, size_t length) noexcept;


/**
 * An implementation of simdutf for a particular CPU architecture.
 *
 * Also used to maintain the currently active implementation. The active implementation is
 * automatically initialized on first use to the most advanced implementation supported by the host.
 */
class implementation {
public:

  /**
   * The name of this implementation.
   *
   *     const implementation *impl = simdutf::active_implementation;
   *     cout << "simdutf is optimized for " << impl->name() << "(" << impl->description() << ")" << endl;
   *
   * @return the name of the implementation, e.g. "haswell", "westmere", "arm64"
   */
  virtual const std::string &name() const { return _name; }

  /**
   * The description of this implementation.
   *
   *     const implementation *impl = simdutf::active_implementation;
   *     cout << "simdutf is optimized for " << impl->name() << "(" << impl->description() << ")" << endl;
   *
   * @return the name of the implementation, e.g. "haswell", "westmere", "arm64"
   */
  virtual const std::string &description() const { return _description; }

  /**
   * The instruction sets this implementation is compiled against
   * and the current CPU match. This function may poll the current CPU/system
   * and should therefore not be called too often if performance is a concern.
   *
   *
   * @return true if the implementation can be safely used on the current system (determined at runtime)
   */
  bool supported_by_runtime_system() const;

  /**
   * This function will try to detect the encoding
   * @param input the string to identify
   * @param length the length of the string in bytes.
   * @return the encoding type detected
   */
  virtual encoding_type autodetect_encoding(const char * input, size_t length) const noexcept;

  /**
   * @private For internal implementation use
   *
   * The instruction sets this implementation is compiled against.
   *
   * @return a mask of all required `internal::instruction_set::` values
   */
  virtual uint32_t required_instruction_sets() const { return _required_instruction_sets; };


  /**
   * Validate the UTF-8 string.
   *
   * Overridden by each implementation.
   *
   * @param buf the string to validate.
   * @param len the length of the string in bytes.
   * @return true if and only if the string is valid UTF-8.
   */
  simdutf_warn_unused virtual bool validate_utf8(const char *buf, size_t len) const noexcept = 0;

  /**
   * Validate the UTF-16 string.
   *
   * Overridden by each implementation.
   *
   * This function is not BOM-aware.
   *
   * @param buf the string to validate.
   * @param len the length of the string in number of char16_t.
   * @return true if and only if the string is valid UTF-16.
   */
  simdutf_warn_unused virtual bool validate_utf16(const char16_t *buf, size_t len) const noexcept = 0;

  /**
   * Convert possibly broken UTF-8 string into UTF-16 string.
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.
   *
   * @param input         the string to convert
   * @param length        the length of the string in bytes
   * @param utf16_buffer  the pointer to buffer that can hold conversion result
   * @return the number of written char16_t; 0 if the input was not valid UTF-8 string
   */
  simdutf_warn_unused virtual size_t convert_utf8_to_utf16(const char * input, size_t length, char16_t* utf8_output) const noexcept = 0;

  /**
   * Convert valid UTF-8 string into UTF-16 string.
   *
   * This function assumes that the input string is valid UTF-8.
   *
   * @param input         the string to convert
   * @param length        the length of the string in bytes
   * @param utf16_buffer  the pointer to buffer that can hold conversion result
   * @return the number of written char16_t
   */
  simdutf_warn_unused virtual size_t convert_valid_utf8_to_utf16(const char * input, size_t length, char16_t* utf16_buffer) const noexcept = 0;

  /**
   * Convert possibly broken UTF-16 string into UTF-8 string.
   *
   * During the conversion also validation of the input string is done.
   * This function is suitable to work with inputs from untrusted sources.
   *
   * This function is not BOM-aware.
   *
   * @param input         the string to convert
   * @param length        the length of the string in words
   * @param utf8_buffer   the pointer to buffer that can hold conversion result
   * @return number of written words; 0 if input is not a valid UTF-16 string
   */
  simdutf_warn_unused virtual size_t convert_utf16_to_utf8(const char16_t * input, size_t length, char* utf8_buffer) const noexcept = 0;

  /**
   * Convert valid UTF-16 string into UTF-8 string.
   *
   * This function assumes that the input string is valid UTF-16.
   *
   * This function is not BOM-aware.
   *
   * @param input         the string to convert
   * @param length        the length of the string in words
   * @param utf8_buffer   the pointer to buffer that can hold conversion result
   * @return number of written words; 0 if conversion is not possible
   */
  simdutf_warn_unused virtual size_t convert_valid_utf16_to_utf8(const char16_t * input, size_t length, char* utf8_buffer) const noexcept = 0;

  /**
   * Count the number of code points (characters) in the string assuming that
   * it is valid.
   *
   * This function assumes that the input string is valid UTF-16.
   *
   * This function is not BOM-aware.
   *
   * @param input         the string to process
   * @param length        the length of the string in words
   * @return number of code points
   */
  simdutf_warn_unused virtual size_t count_utf16(const char16_t * input, size_t length) const noexcept = 0;

  /**
   * Count the number of code points (characters) in the string assuming that
   * it is valid.
   *
   * This function assumes that the input string is valid UTF-8.
   *
   * @param input         the string to process
   * @param length        the length of the string in bytes
   * @return number of code points
   */
  simdutf_warn_unused virtual size_t count_utf8(const char * input, size_t length) const noexcept = 0;



protected:
  /** @private Construct an implementation with the given name and description. For subclasses. */
  simdutf_really_inline implementation(
    std::string_view name,
    std::string_view description,
    uint32_t required_instruction_sets
  ) :
    _name(name),
    _description(description),
    _required_instruction_sets(required_instruction_sets)
  {
  }
  virtual ~implementation()=default;

private:
  /**
   * The name of this implementation.
   */
  const std::string _name;

  /**
   * The description of this implementation.
   */
  const std::string _description;

  /**
   * Instruction sets required for this implementation.
   */
  const uint32_t _required_instruction_sets;
};

/** @private */
namespace internal {

/**
 * The list of available implementations compiled into simdutf.
 */
class available_implementation_list {
public:
  /** Get the list of available implementations compiled into simdutf */
  simdutf_really_inline available_implementation_list() {}
  /** Number of implementations */
  size_t size() const noexcept;
  /** STL const begin() iterator */
  const implementation * const *begin() const noexcept;
  /** STL const end() iterator */
  const implementation * const *end() const noexcept;

  /**
   * Get the implementation with the given name.
   *
   * Case sensitive.
   *
   *     const implementation *impl = simdutf::available_implementations["westmere"];
   *     if (!impl) { exit(1); }
   *     if (!imp->supported_by_runtime_system()) { exit(1); }
   *     simdutf::active_implementation = impl;
   *
   * @param name the implementation to find, e.g. "westmere", "haswell", "arm64"
   * @return the implementation, or nullptr if the parse failed.
   */
  const implementation * operator[](const std::string_view &name) const noexcept {
    for (const implementation * impl : *this) {
      if (impl->name() == name) { return impl; }
    }
    return nullptr;
  }

  /**
   * Detect the most advanced implementation supported by the current host.
   *
   * This is used to initialize the implementation on startup.
   *
   *     const implementation *impl = simdutf::available_implementation::detect_best_supported();
   *     simdutf::active_implementation = impl;
   *
   * @return the most advanced supported implementation for the current host, or an
   *         implementation that returns UNSUPPORTED_ARCHITECTURE if there is no supported
   *         implementation. Will never return nullptr.
   */
  const implementation *detect_best_supported() const noexcept;
};

template<typename T>
class atomic_ptr {
public:
  atomic_ptr(T *_ptr) : ptr{_ptr} {}

  operator const T*() const { return ptr.load(); }
  const T& operator*() const { return *ptr; }
  const T* operator->() const { return ptr.load(); }

  operator T*() { return ptr.load(); }
  T& operator*() { return *ptr; }
  T* operator->() { return ptr.load(); }
  atomic_ptr& operator=(T *_ptr) { ptr = _ptr; return *this; }

private:
  std::atomic<T*> ptr;
};

} // namespace internal

/**
 * The list of available implementations compiled into simdutf.
 */
extern SIMDUTF_DLLIMPORTEXPORT const internal::available_implementation_list available_implementations;

/**
  * The active implementation.
  *
  * Automatically initialized on first use to the most advanced implementation supported by this hardware.
  */
extern SIMDUTF_DLLIMPORTEXPORT internal::atomic_ptr<const implementation> active_implementation;

} // namespace simdutf

#endif // SIMDUTF_IMPLEMENTATION_H
