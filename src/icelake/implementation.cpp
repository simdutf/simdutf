#include "simdutf/icelake/intrinsics.h"

#include "scalar/utf16_to_utf8/valid_utf16_to_utf8.h"
#include "scalar/utf16_to_utf8/utf16_to_utf8.h"
#include "scalar/utf8_to_utf16/valid_utf8_to_utf16.h"
#include "scalar/utf8_to_utf16/utf8_to_utf16.h"
#include "scalar/utf8.h"
#include "scalar/utf16.h"
#include "scalar/latin1.h"
#include "scalar/utf8_to_latin1/valid_utf8_to_latin1.h"
#include "scalar/utf8_to_latin1/utf8_to_latin1.h"

#include "simdutf/icelake/begin.h"
namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {
#ifndef SIMDUTF_ICELAKE_H
#error "icelake.h must be included"
#endif
#include "icelake/icelake_utf8_common.inl.cpp"
#include "icelake/icelake_macros.inl.cpp"
#include "icelake/icelake_from_valid_utf8.inl.cpp"
#include "icelake/icelake_utf8_validation.inl.cpp"
#include "icelake/icelake_from_utf8.inl.cpp"
#include "icelake/icelake_convert_utf16_to_utf32.inl.cpp"
#include "icelake/icelake_convert_utf32_to_utf8.inl.cpp"
#include "icelake/icelake_convert_utf32_to_utf16.inl.cpp"
#include "icelake/icelake_ascii_validation.inl.cpp"
#include "icelake/icelake_utf32_validation.inl.cpp"
#include "icelake/icelake_convert_utf16_to_utf8.inl.cpp"

} // namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf

namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {

simdutf_warn_unused int
implementation::detect_encodings(const char *input,
                                 size_t length) const noexcept {
  // If there is a BOM, then we trust it.
  auto bom_encoding = simdutf::BOM::check_bom(input, length);
  if(bom_encoding != encoding_type::unspecified) { return bom_encoding; }
  if (length % 2 == 0) {
    const char *buf = input;

    const char *start = buf;
    const char *end = input + length;

    bool is_utf8 = true;
    bool is_utf16 = true;
    bool is_utf32 = true;

    int out = 0;

    avx512_utf8_checker checker{};
    __m512i currentmax = _mm512_setzero_si512();
    while (buf + 64 <= end) {
      __m512i in = _mm512_loadu_si512((__m512i *)buf);
      __m512i diff = _mm512_sub_epi16(in, _mm512_set1_epi16(uint16_t(0xD800)));
      __mmask32 surrogates =
          _mm512_cmplt_epu16_mask(diff, _mm512_set1_epi16(uint16_t(0x0800)));
      if (surrogates) {
        is_utf8 = false;

        // Can still be either UTF-16LE or UTF-32 depending on the positions
        // of the surrogates To be valid UTF-32, a surrogate cannot be in the
        // two most significant bytes of any 32-bit word. On the other hand, to
        // be valid UTF-16LE, at least one surrogate must be in the two most
        // significant bytes of a 32-bit word since they always come in pairs in
        // UTF-16LE. Note that we always proceed in multiple of 4 before this
        // point so there is no offset in 32-bit words.

        if ((surrogates & 0xaaaaaaaa) != 0) {
          is_utf32 = false;
          __mmask32 highsurrogates = _mm512_cmplt_epu16_mask(
              diff, _mm512_set1_epi16(uint16_t(0x0400)));
          __mmask32 lowsurrogates = surrogates ^ highsurrogates;
          // high must be followed by low
          if ((highsurrogates << 1) != lowsurrogates) {
            return simdutf::encoding_type::unspecified;
          }

          bool ends_with_high = ((highsurrogates & 0x80000000) != 0);
          if (ends_with_high) {
            buf +=
                31 *
                sizeof(char16_t); // advance only by 31 words so that we start
                                  // with the high surrogate on the next round.
          } else {
            buf += 32 * sizeof(char16_t);
          }
          is_utf16 = validate_utf16le(reinterpret_cast<const char16_t *>(buf),
                                      (end - buf) / sizeof(char16_t));
          if (!is_utf16) {
            return simdutf::encoding_type::unspecified;

          } else {
            return simdutf::encoding_type::UTF16_LE;
          }

        } else {
          is_utf16 = false;
          // Check for UTF-32
          if (length % 4 == 0) {
            const char32_t *input32 = reinterpret_cast<const char32_t *>(buf);
            const char32_t *end32 =
                reinterpret_cast<const char32_t *>(start) + length / 4;
            if (validate_utf32(input32, end32 - input32)) {
              return simdutf::encoding_type::UTF32_LE;
            }
          }
          return simdutf::encoding_type::unspecified;
        }
        break;
      }
      // If no surrogate, validate under other encodings as well

      // UTF-32 validation
      currentmax = _mm512_max_epu32(in, currentmax);

      // UTF-8 validation
      checker.check_next_input(in);

      buf += 64;
    }

    // Check which encodings are possible

    if (is_utf8) {
      size_t current_length = static_cast<size_t>(buf - start);
      if (current_length != length) {
        const __m512i utf8 = _mm512_maskz_loadu_epi8(
            (1ULL << (length - current_length)) - 1, (const __m512i *)buf);
        checker.check_next_input(utf8);
      }
      checker.check_eof();
      if (!checker.errors()) {
        out |= simdutf::encoding_type::UTF8;
      }
    }

    if (is_utf16 && scalar::utf16::validate<endianness::LITTLE>(
                        reinterpret_cast<const char16_t *>(buf),
                        (length - (buf - start)) / 2)) {
      out |= simdutf::encoding_type::UTF16_LE;
    }

    if (is_utf32 && (length % 4 == 0)) {
      currentmax = _mm512_max_epu32(
          _mm512_maskz_loadu_epi8(
              (1ULL << (length - static_cast<size_t>(buf - start))) - 1,
              (const __m512i *)buf),
          currentmax);
      __mmask16 outside_range = _mm512_cmp_epu32_mask(currentmax, _mm512_set1_epi32(0x10ffff),
                                _MM_CMPINT_GT);
      if (outside_range == 0) {
        out |= simdutf::encoding_type::UTF32_LE;
      }
    }

    return out;
  } else if (implementation::validate_utf8(input, length)) {
    return simdutf::encoding_type::UTF8;
  } else {
    return simdutf::encoding_type::unspecified;
  }
}

simdutf_warn_unused bool implementation::validate_utf8(const char *buf, size_t len) const noexcept {
    avx512_utf8_checker checker{};
    const char* ptr = buf;
    const char* end = ptr + len;
    for (; ptr + 64 <= end; ptr += 64) {
        const __m512i utf8 = _mm512_loadu_si512((const __m512i*)ptr);
        checker.check_next_input(utf8);
    }
    {
       const __m512i utf8 = _mm512_maskz_loadu_epi8((1ULL<<(end - ptr))-1, (const __m512i*)ptr);
       checker.check_next_input(utf8);
    }
    checker.check_eof();
    return ! checker.errors();
}

simdutf_warn_unused result implementation::validate_utf8_with_errors(const char *buf, size_t len) const noexcept {
    avx512_utf8_checker checker{};
    const char* ptr = buf;
    const char* end = ptr + len;
    size_t count{0};
    for (; ptr + 64 <= end; ptr += 64) {
      const __m512i utf8 = _mm512_loadu_si512((const __m512i*)ptr);
      checker.check_next_input(utf8);
      if(checker.errors()) {
        if (count != 0) { count--; } // Sometimes the error is only detected in the next chunk
        result res = scalar::utf8::rewind_and_validate_with_errors(reinterpret_cast<const char*>(buf + count), len - count);
        res.count += count;
        return res;
      }
      count += 64;
    }
    {
      const __m512i utf8 = _mm512_maskz_loadu_epi8((1ULL<<(end - ptr))-1, (const __m512i*)ptr);
      checker.check_next_input(utf8);
      if(checker.errors()) {
        if (count != 0) { count--; } // Sometimes the error is only detected in the next chunk
        result res = scalar::utf8::rewind_and_validate_with_errors(reinterpret_cast<const char*>(buf + count), len - count);
        res.count += count;
        return res;
      } else {
        return result(error_code::SUCCESS, len);
      }
    }
}

simdutf_warn_unused bool implementation::validate_ascii(const char *buf, size_t len) const noexcept {
  return icelake::validate_ascii(buf, len);
}

simdutf_warn_unused result implementation::validate_ascii_with_errors(const char *buf, size_t len) const noexcept {
  const char* buf_orig = buf;
  const char* end = buf + len;
  const __m512i ascii = _mm512_set1_epi8((uint8_t)0x80);
  for (; buf + 64 <= end; buf += 64) {
    const __m512i input = _mm512_loadu_si512((const __m512i*)buf);
    __mmask64 notascii = _mm512_cmp_epu8_mask(input, ascii, _MM_CMPINT_NLT);
    if(notascii) {
      return result(error_code::TOO_LARGE, buf - buf_orig + _tzcnt_u64(notascii));
    }
  }
  {
    const __m512i input = _mm512_maskz_loadu_epi8((1ULL<<(end - buf))-1, (const __m512i*)buf);
    __mmask64 notascii = _mm512_cmp_epu8_mask(input, ascii, _MM_CMPINT_NLT);
    if(notascii) {
      return result(error_code::TOO_LARGE, buf - buf_orig + _tzcnt_u64(notascii));
    }
  }
  return result(error_code::SUCCESS, len);
}

simdutf_warn_unused bool implementation::validate_utf16le(const char16_t *buf, size_t len) const noexcept {
    const char16_t *end = buf + len;

    for(;buf + 32 <= end; ) {
      __m512i in = _mm512_loadu_si512((__m512i*)buf);
      __m512i diff = _mm512_sub_epi16(in, _mm512_set1_epi16(uint16_t(0xD800)));
      __mmask32 surrogates = _mm512_cmplt_epu16_mask(diff, _mm512_set1_epi16(uint16_t(0x0800)));
      if(surrogates) {
        __mmask32 highsurrogates = _mm512_cmplt_epu16_mask(diff, _mm512_set1_epi16(uint16_t(0x0400)));
        __mmask32 lowsurrogates = surrogates ^ highsurrogates;
        // high must be followed by low
        if ((highsurrogates << 1) != lowsurrogates) {
           return false;
        }
        bool ends_with_high = ((highsurrogates & 0x80000000) != 0);
        if(ends_with_high) {
          buf += 31; // advance only by 31 words so that we start with the high surrogate on the next round.
        } else {
          buf += 32;
        }
      } else {
        buf += 32;
      }
    }
    if(buf < end) {
      __m512i in = _mm512_maskz_loadu_epi16((1<<(end-buf))-1,(__m512i*)buf);
      __m512i diff = _mm512_sub_epi16(in, _mm512_set1_epi16(uint16_t(0xD800)));
      __mmask32 surrogates = _mm512_cmplt_epu16_mask(diff, _mm512_set1_epi16(uint16_t(0x0800)));
      if(surrogates) {
        __mmask32 highsurrogates = _mm512_cmplt_epu16_mask(diff, _mm512_set1_epi16(uint16_t(0x0400)));
        __mmask32 lowsurrogates = surrogates ^ highsurrogates;
        // high must be followed by low
        if ((highsurrogates << 1) != lowsurrogates) {
           return false;
        }
      }
    }
    return true;
}

simdutf_warn_unused bool implementation::validate_utf16be(const char16_t *buf, size_t len) const noexcept {
   const char16_t *end = buf + len;
   const __m512i byteflip = _mm512_setr_epi64(
            0x0607040502030001,
            0x0e0f0c0d0a0b0809,
            0x0607040502030001,
            0x0e0f0c0d0a0b0809,
            0x0607040502030001,
            0x0e0f0c0d0a0b0809,
            0x0607040502030001,
            0x0e0f0c0d0a0b0809
        );
    for(;buf + 32 <= end; ) {
      __m512i in = _mm512_shuffle_epi8(_mm512_loadu_si512((__m512i*)buf), byteflip);
      __m512i diff = _mm512_sub_epi16(in, _mm512_set1_epi16(uint16_t(0xD800)));
      __mmask32 surrogates = _mm512_cmplt_epu16_mask(diff, _mm512_set1_epi16(uint16_t(0x0800)));
      if(surrogates) {
        __mmask32 highsurrogates = _mm512_cmplt_epu16_mask(diff, _mm512_set1_epi16(uint16_t(0x0400)));
        __mmask32 lowsurrogates = surrogates ^ highsurrogates;
        // high must be followed by low
        if ((highsurrogates << 1) != lowsurrogates) {
           return false;
        }
        bool ends_with_high = ((highsurrogates & 0x80000000) != 0);
        if(ends_with_high) {
          buf += 31; // advance only by 31 words so that we start with the high surrogate on the next round.
        } else {
          buf += 32;
        }
      } else {
        buf += 32;
      }
    }
    if(buf < end) {
      __m512i in = _mm512_shuffle_epi8(_mm512_maskz_loadu_epi16((1<<(end-buf))-1,(__m512i*)buf), byteflip);
      __m512i diff = _mm512_sub_epi16(in, _mm512_set1_epi16(uint16_t(0xD800)));
      __mmask32 surrogates = _mm512_cmplt_epu16_mask(diff, _mm512_set1_epi16(uint16_t(0x0800)));
      if(surrogates) {
        __mmask32 highsurrogates = _mm512_cmplt_epu16_mask(diff, _mm512_set1_epi16(uint16_t(0x0400)));
        __mmask32 lowsurrogates = surrogates ^ highsurrogates;
        // high must be followed by low
        if ((highsurrogates << 1) != lowsurrogates) {
           return false;
        }
      }
    }
    return true;
}

simdutf_warn_unused result implementation::validate_utf16le_with_errors(const char16_t *buf, size_t len) const noexcept {
    const char16_t *start_buf = buf;
    const char16_t *end = buf + len;
    for(;buf + 32 <= end; ) {
      __m512i in = _mm512_loadu_si512((__m512i*)buf);
      __m512i diff = _mm512_sub_epi16(in, _mm512_set1_epi16(uint16_t(0xD800)));
      __mmask32 surrogates = _mm512_cmplt_epu16_mask(diff, _mm512_set1_epi16(uint16_t(0x0800)));
      if(surrogates) {
        __mmask32 highsurrogates = _mm512_cmplt_epu16_mask(diff, _mm512_set1_epi16(uint16_t(0x0400)));
        __mmask32 lowsurrogates = surrogates ^ highsurrogates;
        // high must be followed by low
        if ((highsurrogates << 1) != lowsurrogates) {
          uint32_t extra_low = _tzcnt_u32(lowsurrogates &~(highsurrogates << 1));
          uint32_t extra_high = _tzcnt_u32(highsurrogates &~(lowsurrogates >> 1));
          return result(error_code::SURROGATE, (buf - start_buf) + (extra_low < extra_high ? extra_low : extra_high));
        }
        bool ends_with_high = ((highsurrogates & 0x80000000) != 0);
        if(ends_with_high) {
          buf += 31; // advance only by 31 words so that we start with the high surrogate on the next round.
        } else {
          buf += 32;
        }
      } else {
        buf += 32;
      }
    }
    if(buf < end) {
      __m512i in = _mm512_maskz_loadu_epi16((1<<(end-buf))-1,(__m512i*)buf);
      __m512i diff = _mm512_sub_epi16(in, _mm512_set1_epi16(uint16_t(0xD800)));
      __mmask32 surrogates = _mm512_cmplt_epu16_mask(diff, _mm512_set1_epi16(uint16_t(0x0800)));
      if(surrogates) {
        __mmask32 highsurrogates = _mm512_cmplt_epu16_mask(diff, _mm512_set1_epi16(uint16_t(0x0400)));
        __mmask32 lowsurrogates = surrogates ^ highsurrogates;
        // high must be followed by low
        if ((highsurrogates << 1) != lowsurrogates) {
          uint32_t extra_low = _tzcnt_u32(lowsurrogates &~(highsurrogates << 1));
          uint32_t extra_high = _tzcnt_u32(highsurrogates &~(lowsurrogates >> 1));
          return result(error_code::SURROGATE, (buf - start_buf) + (extra_low < extra_high ? extra_low : extra_high));
        }
      }
    }
    return result(error_code::SUCCESS, len);
}

simdutf_warn_unused result implementation::validate_utf16be_with_errors(const char16_t *buf, size_t len) const noexcept {
    const char16_t *start_buf = buf;
    const char16_t *end = buf + len;
    const __m512i byteflip = _mm512_setr_epi64(
            0x0607040502030001,
            0x0e0f0c0d0a0b0809,
            0x0607040502030001,
            0x0e0f0c0d0a0b0809,
            0x0607040502030001,
            0x0e0f0c0d0a0b0809,
            0x0607040502030001,
            0x0e0f0c0d0a0b0809
        );
    for(;buf + 32 <= end; ) {
      __m512i in = _mm512_shuffle_epi8(_mm512_loadu_si512((__m512i*)buf), byteflip);
      __m512i diff = _mm512_sub_epi16(in, _mm512_set1_epi16(uint16_t(0xD800)));
      __mmask32 surrogates = _mm512_cmplt_epu16_mask(diff, _mm512_set1_epi16(uint16_t(0x0800)));
      if(surrogates) {
        __mmask32 highsurrogates = _mm512_cmplt_epu16_mask(diff, _mm512_set1_epi16(uint16_t(0x0400)));
        __mmask32 lowsurrogates = surrogates ^ highsurrogates;
        // high must be followed by low
        if ((highsurrogates << 1) != lowsurrogates) {
          uint32_t extra_low = _tzcnt_u32(lowsurrogates &~(highsurrogates << 1));
          uint32_t extra_high = _tzcnt_u32(highsurrogates &~(lowsurrogates >> 1));
          return result(error_code::SURROGATE, (buf - start_buf) + (extra_low < extra_high ? extra_low : extra_high));
        }
        bool ends_with_high = ((highsurrogates & 0x80000000) != 0);
        if(ends_with_high) {
          buf += 31; // advance only by 31 words so that we start with the high surrogate on the next round.
        } else {
          buf += 32;
        }
      } else {
        buf += 32;
      }
    }
    if(buf < end) {
      __m512i in = _mm512_shuffle_epi8(_mm512_maskz_loadu_epi16((1<<(end-buf))-1,(__m512i*)buf), byteflip);
      __m512i diff = _mm512_sub_epi16(in, _mm512_set1_epi16(uint16_t(0xD800)));
      __mmask32 surrogates = _mm512_cmplt_epu16_mask(diff, _mm512_set1_epi16(uint16_t(0x0800)));
      if(surrogates) {
        __mmask32 highsurrogates = _mm512_cmplt_epu16_mask(diff, _mm512_set1_epi16(uint16_t(0x0400)));
        __mmask32 lowsurrogates = surrogates ^ highsurrogates;
        // high must be followed by low
        if ((highsurrogates << 1) != lowsurrogates) {
          uint32_t extra_low = _tzcnt_u32(lowsurrogates &~(highsurrogates << 1));
          uint32_t extra_high = _tzcnt_u32(highsurrogates &~(lowsurrogates >> 1));
          return result(error_code::SURROGATE, (buf - start_buf) + (extra_low < extra_high ? extra_low : extra_high));
        }
      }
    }
    return result(error_code::SUCCESS, len);
}

simdutf_warn_unused bool implementation::validate_utf32(const char32_t *buf, size_t len) const noexcept {
  const char32_t * tail = icelake::validate_utf32(buf, len);
  if (tail) {
    return scalar::utf32::validate(tail, len - (tail - buf));
  } else {
    return false;
  }
}

simdutf_warn_unused result implementation::validate_utf32_with_errors(const char32_t *buf, size_t len) const noexcept {

    const char32_t* end = len >= 16 ? buf + len - 16 : nullptr;
    const char32_t* buf_orig = buf;
    while (buf <= end) {
      __m512i utf32 = _mm512_loadu_si512((const __m512i*)buf);
      __mmask16 outside_range = _mm512_cmp_epu32_mask(utf32, _mm512_set1_epi32(0x10ffff),
                                _MM_CMPINT_GT);
      if (outside_range) {
        return result(error_code::TOO_LARGE, buf - buf_orig + _tzcnt_u32(outside_range));
      }

      __m512i utf32_off = _mm512_add_epi32(utf32, _mm512_set1_epi32(0xffff2000));

      __mmask16 surrogate_range = _mm512_cmp_epu32_mask(utf32_off, _mm512_set1_epi32(0xfffff7ff),
                                _MM_CMPINT_GT);
      if (surrogate_range) {
        return result(error_code::SURROGATE, buf - buf_orig + _tzcnt_u32(surrogate_range));
      }
      buf += 16;
    }
    if(buf < buf_orig + len) {
      __m512i utf32 = _mm512_maskz_loadu_epi32(__mmask16((1<<(buf_orig + len - buf))-1),(const __m512i*)buf);
      __mmask16 outside_range = _mm512_cmp_epu32_mask(utf32, _mm512_set1_epi32(0x10ffff),
                                _MM_CMPINT_GT);
      if (outside_range) {
        return result(error_code::TOO_LARGE, buf - buf_orig + _tzcnt_u32(outside_range));
      }
      __m512i utf32_off = _mm512_add_epi32(utf32, _mm512_set1_epi32(0xffff2000));

      __mmask16 surrogate_range = _mm512_cmp_epu32_mask(utf32_off, _mm512_set1_epi32(0xfffff7ff),
                                _MM_CMPINT_GT);
      if (surrogate_range) {
        return result(error_code::SURROGATE, buf - buf_orig + _tzcnt_u32(surrogate_range));
      }
    }

    return result(error_code::SUCCESS, len);
}

simdutf_warn_unused size_t implementation::convert_latin1_to_utf8(const char * buf, size_t len, char* utf8_output) const noexcept {
  return scalar::latin1_to_utf8::convert(buf,len,utf8_output);
}

simdutf_warn_unused size_t implementation::convert_latin1_to_utf16le(const char* buf, size_t len, char16_t* utf16_output) const noexcept {
  return scalar::latin1_to_utf16::convert<endianness::LITTLE>(buf, len, utf16_output);
}

simdutf_warn_unused size_t implementation::convert_latin1_to_utf16be(const char* buf, size_t len, char16_t* utf16_output) const noexcept {
  return scalar::latin1_to_utf16::convert<endianness::BIG>(buf, len, utf16_output);
}

simdutf_warn_unused size_t implementation::convert_latin1_to_utf32(const char* buf, size_t len, char32_t* latin1_output) const noexcept {
  return scalar::latin1_to_utf32::convert(buf,len,latin1_output);
}

simdutf_warn_unused size_t implementation::convert_utf8_to_latin1(const char* buf, size_t len, char* latin1_output) const noexcept {
  return scalar::utf8_to_latin1::convert(buf, len, latin1_output);
}

simdutf_warn_unused result implementation::convert_utf8_to_latin1_with_errors(const char* buf, size_t len, char* latin1_output) const noexcept {
  return scalar::utf8_to_latin1::convert_with_errors(buf, len, latin1_output);
}

simdutf_warn_unused size_t implementation::convert_valid_utf8_to_latin1(const char* buf, size_t len, char* latin1_output) const noexcept {
  return scalar::utf8_to_latin1::convert_valid(buf, len, latin1_output);
}

simdutf_warn_unused size_t implementation::convert_utf8_to_utf16le(const char* buf, size_t len, char16_t* utf16_output) const noexcept {
  utf8_to_utf16_result ret = fast_avx512_convert_utf8_to_utf16<endianness::LITTLE>(buf, len, utf16_output);
  if (ret.second == nullptr) {
    return 0;
  }
  return ret.second - utf16_output;
}

simdutf_warn_unused size_t implementation::convert_utf8_to_utf16be(const char* buf, size_t len, char16_t* utf16_output) const noexcept {
  utf8_to_utf16_result ret = fast_avx512_convert_utf8_to_utf16<endianness::BIG>(buf, len, utf16_output);
  if (ret.second == nullptr) {
    return 0;
  }
  return ret.second - utf16_output;
}

simdutf_warn_unused result implementation::convert_utf8_to_utf16le_with_errors(const char* buf, size_t len, char16_t* utf16_output) const noexcept {
   return fast_avx512_convert_utf8_to_utf16_with_errors<endianness::LITTLE>(buf, len, utf16_output);
}

simdutf_warn_unused result implementation::convert_utf8_to_utf16be_with_errors(const char* buf, size_t len, char16_t* utf16_output) const noexcept {
   return fast_avx512_convert_utf8_to_utf16_with_errors<endianness::BIG>(buf, len, utf16_output);
}

simdutf_warn_unused size_t implementation::convert_valid_utf8_to_utf16le(const char* buf, size_t len, char16_t* utf16_output) const noexcept {
  utf8_to_utf16_result ret = icelake::valid_utf8_to_fixed_length<endianness::LITTLE, char16_t>(buf, len, utf16_output);
  size_t saved_bytes = ret.second - utf16_output;
  const char* end = buf + len;
  if (ret.first == end) {
    return saved_bytes;
  }

  // Note: AVX512 procedure looks up 4 bytes forward, and
  //       correctly converts multi-byte chars even if their
  //       continuation bytes lie outsiede 16-byte window.
  //       It meas, we have to skip continuation bytes from
  //       the beginning ret.first, as they were already consumed.
  while (ret.first != end && ((uint8_t(*ret.first) & 0xc0) == 0x80)) {
      ret.first += 1;
  }

  if (ret.first != end) {
    const size_t scalar_saved_bytes = scalar::utf8_to_utf16::convert_valid<endianness::LITTLE>(
                                        ret.first, len - (ret.first - buf), ret.second);
    if (scalar_saved_bytes == 0) { return 0; }
    saved_bytes += scalar_saved_bytes;
  }

  return saved_bytes;
}

simdutf_warn_unused size_t implementation::convert_valid_utf8_to_utf16be(const char* buf, size_t len, char16_t* utf16_output) const noexcept {
  utf8_to_utf16_result ret = icelake::valid_utf8_to_fixed_length<endianness::BIG, char16_t>(buf, len, utf16_output);
  size_t saved_bytes = ret.second - utf16_output;
  const char* end = buf + len;
  if (ret.first == end) {
    return saved_bytes;
  }

  // Note: AVX512 procedure looks up 4 bytes forward, and
  //       correctly converts multi-byte chars even if their
  //       continuation bytes lie outsiede 16-byte window.
  //       It meas, we have to skip continuation bytes from
  //       the beginning ret.first, as they were already consumed.
  while (ret.first != end && ((uint8_t(*ret.first) & 0xc0) == 0x80)) {
      ret.first += 1;
  }

  if (ret.first != end) {
    const size_t scalar_saved_bytes = scalar::utf8_to_utf16::convert_valid<endianness::BIG>(
                                        ret.first, len - (ret.first - buf), ret.second);
    if (scalar_saved_bytes == 0) { return 0; }
    saved_bytes += scalar_saved_bytes;
  }

  return saved_bytes;
}


simdutf_warn_unused size_t implementation::convert_utf8_to_utf32(const char* buf, size_t len, char32_t* utf32_out) const noexcept {
  uint32_t * utf32_output = reinterpret_cast<uint32_t *>(utf32_out);
  utf8_to_utf32_result ret = icelake::validating_utf8_to_fixed_length<endianness::LITTLE, uint32_t>(buf, len, utf32_output);
  if (ret.second == nullptr)
    return 0;

  size_t saved_bytes = ret.second - utf32_output;
  const char* end = buf + len;
  if (ret.first == end) {
    return saved_bytes;
  }

  // Note: the AVX512 procedure looks up 4 bytes forward, and
  //       correctly converts multi-byte chars even if their
  //       continuation bytes lie outside 16-byte window.
  //       It means, we have to skip continuation bytes from
  //       the beginning ret.first, as they were already consumed.
  while (ret.first != end and ((uint8_t(*ret.first) & 0xc0) == 0x80)) {
      ret.first += 1;
  }

  if (ret.first != end) {
    const size_t scalar_saved_bytes = scalar::utf8_to_utf32::convert(
                                        ret.first, len - (ret.first - buf), utf32_out + saved_bytes);
    if (scalar_saved_bytes == 0) { return 0; }
    saved_bytes += scalar_saved_bytes;
  }

  return saved_bytes;
}

simdutf_warn_unused result implementation::convert_utf8_to_utf32_with_errors(const char* buf, size_t len, char32_t* utf32) const noexcept {
  uint32_t * utf32_output = reinterpret_cast<uint32_t *>(utf32);
  auto ret = icelake::validating_utf8_to_fixed_length_with_constant_checks<endianness::LITTLE, uint32_t>(buf, len, utf32_output);
  if (!std::get<2>(ret)) {
    auto new_buf = std::get<0>(ret);
    // rewind_and_convert_with_errors will seek a potential error from new_buf onward,
    // with the ability to go back up to new_buf - buf bytes, and read len - (new_buf - buf) bytes forward.
    result res = scalar::utf8_to_utf32::rewind_and_convert_with_errors(new_buf - buf, new_buf, len - (new_buf - buf), reinterpret_cast<char32_t *>(std::get<1>(ret)));
    res.count += (std::get<0>(ret) - buf);
    return res;
  }
  size_t saved_bytes = std::get<1>(ret) - utf32_output;
  const char* end = buf + len;
  if (std::get<0>(ret) == end) {
    return {simdutf::SUCCESS, saved_bytes};
  }

  // Note: the AVX512 procedure looks up 4 bytes forward, and
  //       correctly converts multi-byte chars even if their
  //       continuation bytes lie outside 16-byte window.
  //       It means, we have to skip continuation bytes from
  //       the beginning ret.first, as they were already consumed.
  while (std::get<0>(ret) != end and ((uint8_t(*std::get<0>(ret)) & 0xc0) == 0x80)) {
      std::get<0>(ret) += 1;
  }

  if (std::get<0>(ret) != end) {
    auto scalar_result = scalar::utf8_to_utf32::convert_with_errors(
                                        std::get<0>(ret), len - (std::get<0>(ret) - buf), reinterpret_cast<char32_t *>(utf32_output) + saved_bytes);
    if (scalar_result.error != simdutf::SUCCESS) {
      scalar_result.count +=  (std::get<0>(ret) - buf);
    } else {
      scalar_result.count += saved_bytes;
    }
    return scalar_result;
  }

  return {simdutf::SUCCESS, size_t(std::get<1>(ret) - utf32_output)};
}


simdutf_warn_unused size_t implementation::convert_valid_utf8_to_utf32(const char* buf, size_t len, char32_t* utf32_out) const noexcept {
  uint32_t * utf32_output = reinterpret_cast<uint32_t *>(utf32_out);
  utf8_to_utf32_result ret = icelake::valid_utf8_to_fixed_length<endianness::LITTLE, uint32_t>(buf, len, utf32_output);
  size_t saved_bytes = ret.second - utf32_output;
  const char* end = buf + len;
  if (ret.first == end) {
    return saved_bytes;
  }

  // Note: AVX512 procedure looks up 4 bytes forward, and
  //       correctly converts multi-byte chars even if their
  //       continuation bytes lie outsiede 16-byte window.
  //       It meas, we have to skip continuation bytes from
  //       the beginning ret.first, as they were already consumed.
  while (ret.first != end && ((uint8_t(*ret.first) & 0xc0) == 0x80)) {
      ret.first += 1;
  }

  if (ret.first != end) {
    const size_t scalar_saved_bytes = scalar::utf8_to_utf32::convert_valid(
                                        ret.first, len - (ret.first - buf), utf32_out + saved_bytes);
    if (scalar_saved_bytes == 0) { return 0; }
    saved_bytes += scalar_saved_bytes;
  }

  return saved_bytes;
}


simdutf_warn_unused size_t implementation::convert_utf16le_to_latin1(const char16_t* buf, size_t len, char* latin1_output) const noexcept {
  return scalar::utf16_to_latin1::convert<endianness::LITTLE>(buf, len, latin1_output);
}

simdutf_warn_unused size_t implementation::convert_utf16be_to_latin1(const char16_t* buf, size_t len, char* latin1_output) const noexcept {
  return scalar::utf16_to_latin1::convert<endianness::BIG>(buf, len, latin1_output);
}

simdutf_warn_unused result implementation::convert_utf16le_to_latin1_with_errors(const char16_t* buf, size_t len, char* latin1_output) const noexcept {
  return scalar::utf16_to_latin1::convert_with_errors<endianness::LITTLE>(buf, len, latin1_output);
}

simdutf_warn_unused result implementation::convert_utf16be_to_latin1_with_errors(const char16_t* buf, size_t len, char* latin1_output) const noexcept {
  return scalar::utf16_to_latin1::convert_with_errors<endianness::BIG>(buf, len, latin1_output);
}

simdutf_warn_unused size_t implementation::convert_valid_utf16be_to_latin1(const char16_t* buf, size_t len, char* latin1_output) const noexcept {
  return scalar::utf16_to_latin1::convert_valid<endianness::BIG>(buf, len, latin1_output);
}

simdutf_warn_unused size_t implementation::convert_valid_utf16le_to_latin1(const char16_t* buf, size_t len, char* latin1_output) const noexcept {
  return scalar::utf16_to_latin1::convert_valid<endianness::LITTLE>(buf, len, latin1_output);
}

simdutf_warn_unused size_t implementation::convert_utf16le_to_utf8(const char16_t* buf, size_t len, char* utf8_output) const noexcept {
  size_t outlen;
  size_t inlen = utf16_to_utf8_avx512i<endianness::LITTLE>(buf, len, (unsigned char*)utf8_output, &outlen);
  if(inlen != len) { return 0; }
  return outlen;
}

simdutf_warn_unused size_t implementation::convert_utf16be_to_utf8(const char16_t* buf, size_t len, char* utf8_output) const noexcept {
  size_t outlen;
  size_t inlen = utf16_to_utf8_avx512i<endianness::BIG>(buf, len, (unsigned char*)utf8_output, &outlen);
  if(inlen != len) { return 0; }
  return outlen;
}

simdutf_warn_unused result implementation::convert_utf16le_to_utf8_with_errors(const char16_t* buf, size_t len, char* utf8_output) const noexcept {
  size_t outlen;
  size_t inlen = utf16_to_utf8_avx512i<endianness::LITTLE>(buf, len, (unsigned char*)utf8_output, &outlen);
  if(inlen != len) {
    result res = scalar::utf16_to_utf8::convert_with_errors<endianness::LITTLE>(buf + inlen, len - outlen, utf8_output + outlen);
    res.count += inlen;
    return res;
  }
  return {simdutf::SUCCESS, outlen};
}

simdutf_warn_unused result implementation::convert_utf16be_to_utf8_with_errors(const char16_t* buf, size_t len, char* utf8_output) const noexcept {
  size_t outlen;
  size_t inlen = utf16_to_utf8_avx512i<endianness::BIG>(buf, len, (unsigned char*)utf8_output, &outlen);
  if(inlen != len) {
    result res = scalar::utf16_to_utf8::convert_with_errors<endianness::BIG>(buf + inlen, len - outlen, utf8_output + outlen);
    res.count += inlen;
    return res;
  }
  return {simdutf::SUCCESS, outlen};
}

simdutf_warn_unused size_t implementation::convert_valid_utf16le_to_utf8(const char16_t* buf, size_t len, char* utf8_output) const noexcept {
  return convert_utf16le_to_utf8(buf, len, utf8_output);
}

simdutf_warn_unused size_t implementation::convert_valid_utf16be_to_utf8(const char16_t* buf, size_t len, char* utf8_output) const noexcept {
  return convert_utf16be_to_utf8(buf, len, utf8_output);
}

simdutf_warn_unused size_t implementation::convert_utf32_to_latin1(const char32_t* buf, size_t len, char* latin1_output) const noexcept {
  return scalar::utf32_to_latin1::convert(buf,len,latin1_output);
}

simdutf_warn_unused result implementation::convert_utf32_to_latin1_with_errors(const char32_t* buf, size_t len, char* latin1_output) const noexcept {
  return scalar::utf32_to_latin1::convert_with_errors(buf,len,latin1_output);
}

simdutf_warn_unused size_t implementation::convert_valid_utf32_to_latin1(const char32_t* buf, size_t len, char* latin1_output) const noexcept {
  return scalar::utf32_to_latin1::convert_valid(buf,len,latin1_output);
}


simdutf_warn_unused size_t implementation::convert_utf32_to_utf8(const char32_t* buf, size_t len, char* utf8_output) const noexcept {
  std::pair<const char32_t*, char*> ret = avx512_convert_utf32_to_utf8(buf, len, utf8_output);
  if (ret.first == nullptr) { return 0; }
  size_t saved_bytes = ret.second - utf8_output;
  if (ret.first != buf + len) {
    const size_t scalar_saved_bytes = scalar::utf32_to_utf8::convert(
                                        ret.first, len - (ret.first - buf), ret.second);
    if (scalar_saved_bytes == 0) { return 0; }
    saved_bytes += scalar_saved_bytes;
  }
  return saved_bytes;
}

simdutf_warn_unused result implementation::convert_utf32_to_utf8_with_errors(const char32_t* buf, size_t len, char* utf8_output) const noexcept {
  // ret.first.count is always the position in the buffer, not the number of words written even if finished
  std::pair<result, char*> ret = icelake::avx512_convert_utf32_to_utf8_with_errors(buf, len, utf8_output);
  if (ret.first.count != len) {
    result scalar_res = scalar::utf32_to_utf8::convert_with_errors(
                                        buf + ret.first.count, len - ret.first.count, ret.second);
    if (scalar_res.error) {
      scalar_res.count += ret.first.count;
      return scalar_res;
    } else {
      ret.second += scalar_res.count;
    }
  }
  ret.first.count = ret.second - utf8_output;   // Set count to the number of 8-bit words written
  return ret.first;
}

simdutf_warn_unused size_t implementation::convert_valid_utf32_to_utf8(const char32_t* buf, size_t len, char* utf8_output) const noexcept {
  return convert_utf32_to_utf8(buf, len, utf8_output);
}

simdutf_warn_unused size_t implementation::convert_utf32_to_utf16le(const char32_t* buf, size_t len, char16_t* utf16_output) const noexcept {
  std::pair<const char32_t*, char16_t*> ret = avx512_convert_utf32_to_utf16<endianness::LITTLE>(buf, len, utf16_output);
  if (ret.first == nullptr) { return 0; }
  size_t saved_bytes = ret.second - utf16_output;
  if (ret.first != buf + len) {
    const size_t scalar_saved_bytes = scalar::utf32_to_utf16::convert<endianness::LITTLE>(
                                        ret.first, len - (ret.first - buf), ret.second);
    if (scalar_saved_bytes == 0) { return 0; }
    saved_bytes += scalar_saved_bytes;
  }
  return saved_bytes;
}

simdutf_warn_unused size_t implementation::convert_utf32_to_utf16be(const char32_t* buf, size_t len, char16_t* utf16_output) const noexcept {
  std::pair<const char32_t*, char16_t*> ret = avx512_convert_utf32_to_utf16<endianness::BIG>(buf, len, utf16_output);
  if (ret.first == nullptr) { return 0; }
  size_t saved_bytes = ret.second - utf16_output;
  if (ret.first != buf + len) {
    const size_t scalar_saved_bytes = scalar::utf32_to_utf16::convert<endianness::BIG>(
                                        ret.first, len - (ret.first - buf), ret.second);
    if (scalar_saved_bytes == 0) { return 0; }
    saved_bytes += scalar_saved_bytes;
  }
  return saved_bytes;
}

simdutf_warn_unused result implementation::convert_utf32_to_utf16le_with_errors(const char32_t* buf, size_t len, char16_t* utf16_output) const noexcept {
  // ret.first.count is always the position in the buffer, not the number of words written even if finished
  std::pair<result, char16_t*> ret = avx512_convert_utf32_to_utf16_with_errors<endianness::LITTLE>(buf, len, utf16_output);
  if (ret.first.count != len) {
    result scalar_res = scalar::utf32_to_utf16::convert_with_errors<endianness::LITTLE>(
                                        buf + ret.first.count, len - ret.first.count, ret.second);
    if (scalar_res.error) {
      scalar_res.count += ret.first.count;
      return scalar_res;
    } else {
      ret.second += scalar_res.count;
    }
  }
  ret.first.count = ret.second - utf16_output;   // Set count to the number of 8-bit words written
  return ret.first;
}

simdutf_warn_unused result implementation::convert_utf32_to_utf16be_with_errors(const char32_t* buf, size_t len, char16_t* utf16_output) const noexcept {
  // ret.first.count is always the position in the buffer, not the number of words written even if finished
  std::pair<result, char16_t*> ret = avx512_convert_utf32_to_utf16_with_errors<endianness::BIG>(buf, len, utf16_output);
  if (ret.first.count != len) {
    result scalar_res = scalar::utf32_to_utf16::convert_with_errors<endianness::BIG>(
                                        buf + ret.first.count, len - ret.first.count, ret.second);
    if (scalar_res.error) {
      scalar_res.count += ret.first.count;
      return scalar_res;
    } else {
      ret.second += scalar_res.count;
    }
  }
  ret.first.count = ret.second - utf16_output;   // Set count to the number of 8-bit words written
  return ret.first;
}

simdutf_warn_unused size_t implementation::convert_valid_utf32_to_utf16le(const char32_t* buf, size_t len, char16_t* utf16_output) const noexcept {
  return convert_utf32_to_utf16le(buf, len, utf16_output);
}

simdutf_warn_unused size_t implementation::convert_valid_utf32_to_utf16be(const char32_t* buf, size_t len, char16_t* utf16_output) const noexcept {
  return convert_utf32_to_utf16be(buf, len, utf16_output);
}

simdutf_warn_unused size_t implementation::convert_utf16le_to_utf32(const char16_t* buf, size_t len, char32_t* utf32_output) const noexcept {
  std::tuple<const char16_t*, char32_t*, bool> ret = icelake::convert_utf16_to_utf32<endianness::LITTLE>(buf, len, utf32_output);
  if (!std::get<2>(ret)) { return 0; }
  size_t saved_bytes = std::get<1>(ret) - utf32_output;
  if (std::get<0>(ret) != buf + len) {
    const size_t scalar_saved_bytes = scalar::utf16_to_utf32::convert<endianness::LITTLE>(
                                        std::get<0>(ret), len - (std::get<0>(ret) - buf), std::get<1>(ret));
    if (scalar_saved_bytes == 0) { return 0; }
    saved_bytes += scalar_saved_bytes;
  }
  return saved_bytes;
}

simdutf_warn_unused size_t implementation::convert_utf16be_to_utf32(const char16_t* buf, size_t len, char32_t* utf32_output) const noexcept {
  std::tuple<const char16_t*, char32_t*, bool> ret = icelake::convert_utf16_to_utf32<endianness::BIG>(buf, len, utf32_output);
  if (!std::get<2>(ret)) { return 0; }
  size_t saved_bytes = std::get<1>(ret) - utf32_output;
  if (std::get<0>(ret) != buf + len) {
    const size_t scalar_saved_bytes = scalar::utf16_to_utf32::convert<endianness::BIG>(
                                        std::get<0>(ret), len - (std::get<0>(ret) - buf), std::get<1>(ret));
    if (scalar_saved_bytes == 0) { return 0; }
    saved_bytes += scalar_saved_bytes;
  }
  return saved_bytes;
}

simdutf_warn_unused result implementation::convert_utf16le_to_utf32_with_errors(const char16_t* buf, size_t len, char32_t* utf32_output) const noexcept {
  std::tuple<const char16_t*, char32_t*, bool> ret = icelake::convert_utf16_to_utf32<endianness::LITTLE>(buf, len, utf32_output);
  if (!std::get<2>(ret)) {
    result scalar_res = scalar::utf16_to_utf32::convert_with_errors<endianness::LITTLE>(
                                        std::get<0>(ret), len - (std::get<0>(ret) - buf), std::get<1>(ret));
    scalar_res.count += (std::get<0>(ret) - buf);
    return scalar_res;
  }
  size_t saved_bytes = std::get<1>(ret) - utf32_output;
  if (std::get<0>(ret) != buf + len) {
    result scalar_res = scalar::utf16_to_utf32::convert_with_errors<endianness::LITTLE>(
                                        std::get<0>(ret), len - (std::get<0>(ret) - buf), std::get<1>(ret));
    if (scalar_res.error) {
      scalar_res.count += (std::get<0>(ret) - buf);
      return scalar_res;
    } else {
      scalar_res.count += saved_bytes;
      return scalar_res;
    }
  }
  return simdutf::result(simdutf::SUCCESS, saved_bytes);
}

simdutf_warn_unused result implementation::convert_utf16be_to_utf32_with_errors(const char16_t* buf, size_t len, char32_t* utf32_output) const noexcept {
  std::tuple<const char16_t*, char32_t*, bool> ret = icelake::convert_utf16_to_utf32<endianness::BIG>(buf, len, utf32_output);
  if (!std::get<2>(ret)) {
    result scalar_res = scalar::utf16_to_utf32::convert_with_errors<endianness::BIG>(
                                        std::get<0>(ret), len - (std::get<0>(ret) - buf), std::get<1>(ret));
    scalar_res.count += (std::get<0>(ret) - buf);
    return scalar_res;
  }
  size_t saved_bytes = std::get<1>(ret) - utf32_output;
  if (std::get<0>(ret) != buf + len) {
    result scalar_res = scalar::utf16_to_utf32::convert_with_errors<endianness::BIG>(
                                        std::get<0>(ret), len - (std::get<0>(ret) - buf), std::get<1>(ret));
    if (scalar_res.error) {
      scalar_res.count += (std::get<0>(ret) - buf);
      return scalar_res;
    } else {
      scalar_res.count += saved_bytes;
      return scalar_res;
    }
  }
  return simdutf::result(simdutf::SUCCESS, saved_bytes);
}

simdutf_warn_unused size_t implementation::convert_valid_utf16le_to_utf32(const char16_t* buf, size_t len, char32_t* utf32_output) const noexcept {
  std::tuple<const char16_t*, char32_t*, bool> ret = icelake::convert_utf16_to_utf32<endianness::LITTLE>(buf, len, utf32_output);
  if (!std::get<2>(ret)) { return 0; }
  size_t saved_bytes = std::get<1>(ret) - utf32_output;
  if (std::get<0>(ret) != buf + len) {
    const size_t scalar_saved_bytes = scalar::utf16_to_utf32::convert<endianness::LITTLE>(
                                        std::get<0>(ret), len - (std::get<0>(ret) - buf), std::get<1>(ret));
    if (scalar_saved_bytes == 0) { return 0; }
    saved_bytes += scalar_saved_bytes;
  }
  return saved_bytes;
}

simdutf_warn_unused size_t implementation::convert_valid_utf16be_to_utf32(const char16_t* buf, size_t len, char32_t* utf32_output) const noexcept {
  std::tuple<const char16_t*, char32_t*, bool> ret = icelake::convert_utf16_to_utf32<endianness::BIG>(buf, len, utf32_output);
  if (!std::get<2>(ret)) { return 0; }
  size_t saved_bytes = std::get<1>(ret) - utf32_output;
  if (std::get<0>(ret) != buf + len) {
    const size_t scalar_saved_bytes = scalar::utf16_to_utf32::convert<endianness::BIG>(
                                        std::get<0>(ret), len - (std::get<0>(ret) - buf), std::get<1>(ret));
    if (scalar_saved_bytes == 0) { return 0; }
    saved_bytes += scalar_saved_bytes;
  }
  return saved_bytes;
}

void implementation::change_endianness_utf16(const char16_t * input, size_t length, char16_t * output) const noexcept {
  size_t pos = 0;
  const __m512i byteflip = _mm512_setr_epi64(
            0x0607040502030001,
            0x0e0f0c0d0a0b0809,
            0x0607040502030001,
            0x0e0f0c0d0a0b0809,
            0x0607040502030001,
            0x0e0f0c0d0a0b0809,
            0x0607040502030001,
            0x0e0f0c0d0a0b0809
        );
  while (pos + 32 <= length) {
    __m512i utf16 = _mm512_loadu_si512((const __m512i*)(input + pos));
    utf16 = _mm512_shuffle_epi8(utf16, byteflip);
    _mm512_storeu_si512(output + pos, utf16);
    pos += 32;
  }
  if(pos < length) {
    __mmask32 m((1<< (length - pos))-1);
    __m512i utf16 = _mm512_maskz_loadu_epi16(m, (const __m512i*)(input + pos));
    utf16 = _mm512_shuffle_epi8(utf16, byteflip);
    _mm512_mask_storeu_epi16(output + pos, m, utf16);
  }
}


simdutf_warn_unused size_t implementation::count_utf16le(const char16_t * input, size_t length) const noexcept {
  const char16_t* end = length >= 32 ? input + length - 32 : nullptr;
  const char16_t* ptr = input;

  const __m512i low = _mm512_set1_epi16((uint16_t)0xdc00);
  const __m512i high = _mm512_set1_epi16((uint16_t)0xdfff);

  size_t count{0};

  while (ptr <= end) {
    __m512i utf16 = _mm512_loadu_si512((const __m512i*)ptr);
    ptr += 32;
    uint64_t not_high_surrogate = static_cast<uint64_t>(_mm512_cmpgt_epu16_mask(utf16, high) | _mm512_cmplt_epu16_mask(utf16, low));
    count += count_ones(not_high_surrogate);
  }

  return count + scalar::utf16::count_code_points<endianness::LITTLE>(ptr, length - (ptr - input));
}

simdutf_warn_unused size_t implementation::count_utf16be(const char16_t * input, size_t length) const noexcept {
  const char16_t* end = length >= 32 ? input + length - 32 : nullptr;
  const char16_t* ptr = input;

  const __m512i low = _mm512_set1_epi16((uint16_t)0xdc00);
  const __m512i high = _mm512_set1_epi16((uint16_t)0xdfff);

  size_t count{0};
  const __m512i byteflip = _mm512_setr_epi64(
            0x0607040502030001,
            0x0e0f0c0d0a0b0809,
            0x0607040502030001,
            0x0e0f0c0d0a0b0809,
            0x0607040502030001,
            0x0e0f0c0d0a0b0809,
            0x0607040502030001,
            0x0e0f0c0d0a0b0809
        );
  while (ptr <= end) {
    __m512i utf16 = _mm512_shuffle_epi8(_mm512_loadu_si512((__m512i*)ptr), byteflip);
    ptr += 32;
    uint64_t not_high_surrogate = static_cast<uint64_t>(_mm512_cmpgt_epu16_mask(utf16, high) | _mm512_cmplt_epu16_mask(utf16, low));
    count += count_ones(not_high_surrogate);
  }

  return count + scalar::utf16::count_code_points<endianness::BIG>(ptr, length - (ptr - input));
}


simdutf_warn_unused size_t implementation::count_utf8(const char * input, size_t length) const noexcept {
  const uint8_t *str = reinterpret_cast<const uint8_t *>(input);
  __m512i two_leading_bits_mask = _mm512_set1_epi8(static_cast<unsigned char>(0xC0)); // 0xC0 is 1100 0000 in binary
  size_t answer = length / sizeof(__m512i) * sizeof(__m512i); // Number of 512-bit chunks that fits into the length.
  size_t i = 0;
  __m512i eight_64bits = _mm512_setzero_si512();
  while (i + sizeof(__m512i) <= length) {
    __m512i runner = _mm512_setzero_si512();
    size_t iterations = (length - i) / sizeof(__m512i);
    if (iterations > 255) {
      iterations = 255;
    }
    size_t max_i = i + iterations * sizeof(__m512i) - sizeof(__m512i);
/*     for (; i + 4*sizeof(__m512i) <= max_i; i += 4*sizeof(__m512i)) {
            // Load four __m512i vectors
            __m512i input1 = _mm512_loadu_si512((const __m512i *)(str + i));
            __m512i input2 = _mm512_loadu_si512((const __m512i *)(str + i + sizeof(__m512i)));
            __m512i input3 = _mm512_loadu_si512((const __m512i *)(str + i + 2*sizeof(__m512i)));
            __m512i input4 = _mm512_loadu_si512((const __m512i *)(str + i + 3*sizeof(__m512i)));

            __m512i input12 = _mm512_add_epi8( // Add up the presence of continuation bytes in input 1 & 2 per byte element
                                              _mm512_mask_set1_epi8( // 
                                                                    _mm512_setzero_si512(),
                                                                    _mm512_cmpeq_epi8_mask( // 64-bit mask where 1 = continuation byte, 0 = non-ASCII
                                                                                            _mm512_set1_epi8(0x02),
                                                                                            _mm512_srli_epi16(input1, 6)), //we want to isolate the leading first two bytes
                                                                    0xFF),
                                                
                                                _mm512_mask_set1_epi8(_mm512_setzero_si512(),
                                                                    _mm512_cmpeq_epi8_mask( // 64-bit mask where 1 = ASCII, 0 = non-ASCII
                                                                                            _mm512_set1_epi8(0x02),
                                                                                            _mm512_srli_epi16(input2, 6)),
                                                                    0xFF)
                                              );


            __m512i input23 = _mm512_add_epi8( // add up the presence of ASCII bytes in input 1 & 2 per byte element
                                              _mm512_mask_set1_epi8(_mm512_setzero_si512(),
                                                                    _mm512_cmpeq_epi8_mask( // 64-bit mask where 1 = ASCII, 0 = non-ASCII
                                                                                            _mm512_set1_epi8(0x02),
                                                                                            _mm512_srli_epi16(input3, 6)),                                                                                            
                                                                    0xFF),
                                                
                                                _mm512_mask_set1_epi8(_mm512_setzero_si512(),
                                                                    _mm512_cmpeq_epi8_mask( // 64-bit mask where 1 = ASCII, 0 = non-ASCII
                                                                                            _mm512_set1_epi8(0x02),
                                                                                            _mm512_srli_epi16(input4, 6)),
                                                                    0xFF)
                                              );                                            

            __m512i input1234 = _mm512_add_epi8(input12, input23);
            runner = _mm512_sub_epi8(runner, input1234); // you have your runner


    }
 */
    for (; i <= max_i; i += sizeof(__m512i)) {
      __m512i more_input = _mm512_loadu_si512((const __m512i *)(str + i));

      // __mmask64 mask = _mm512_cmpgt_epi8_mask(_mm512_setzero_si512(), more_input);
        __mmask64 mask = _mm512_cmpeq_epi8_mask( // 64-bit mask so that 1 = continuation bytes, 0 = non-continuation
                                            _mm512_set1_epi8(0x80), //a packed 512-bits vector whose bytes are all 1000 0000
                                            // _mm512_srli_epi16(more_input, 6));// shift every byte in avx512 vector six places to the right . e.g. 
                                            _mm512_and_si512( // get only two first leading bits of packed byte
                                                            more_input, 
                                                            two_leading_bits_mask)
                                            );

      __m512i noncontinuation_bytes = _mm512_mask_set1_epi8(_mm512_setzero_si512(), mask, 0xFF); //sets 0xFF for non-continuation bytes, 0x00 otherwise
      runner = _mm512_sub_epi8(runner, noncontinuation_bytes); //add number of non-continuation to runner
    }

    eight_64bits = _mm512_add_epi64(eight_64bits, _mm512_sad_epu8(runner, _mm512_setzero_si512()));
  }

  __m256i first_half = _mm512_extracti64x4_epi64(eight_64bits, 0);
  __m256i second_half = _mm512_extracti64x4_epi64(eight_64bits, 1);
  answer += (size_t)_mm256_extract_epi64(first_half, 0) +
            (size_t)_mm256_extract_epi64(first_half, 1) +
            (size_t)_mm256_extract_epi64(first_half, 2) +
            (size_t)_mm256_extract_epi64(first_half, 3) +
            (size_t)_mm256_extract_epi64(second_half, 0) +
            (size_t)_mm256_extract_epi64(second_half, 1) +
            (size_t)_mm256_extract_epi64(second_half, 2) +
            (size_t)_mm256_extract_epi64(second_half, 3);
  // return answer + scalar_utf8_length(str + i, length - i);

  answer = answer + scalar::utf8::count_code_points(reinterpret_cast<const char *>(str + i), length - i);
  return answer;
}

simdutf_warn_unused size_t implementation::latin1_length_from_utf8(const char* buf, size_t len) const noexcept {
  return scalar::utf8::latin1_length_from_utf8(buf,len);
}

simdutf_warn_unused size_t implementation::latin1_length_from_utf16(size_t length) const noexcept {
  return scalar::utf16::latin1_length_from_utf16(length);
}

simdutf_warn_unused size_t implementation::latin1_length_from_utf32( size_t length) const noexcept {
  return scalar::utf32::latin1_length_from_utf32(length);
}

simdutf_warn_unused size_t implementation::utf8_length_from_utf16le(const char16_t * input, size_t length) const noexcept {
  const char16_t* end = length >= 32 ? input + length - 32 : nullptr;
  const char16_t* ptr = input;

  const __m512i v_007f = _mm512_set1_epi16((uint16_t)0x007f);
  const __m512i v_07ff = _mm512_set1_epi16((uint16_t)0x07ff);
  const __m512i v_dfff = _mm512_set1_epi16((uint16_t)0xdfff);
  const __m512i v_d800 = _mm512_set1_epi16((uint16_t)0xd800);

  size_t count{0};

  while (ptr <= end) {
    __m512i utf16 = _mm512_loadu_si512((const __m512i*)ptr);
    ptr += 32;
    __mmask32 ascii_bitmask = _mm512_cmple_epu16_mask(utf16, v_007f);
    __mmask32 two_bytes_bitmask = _mm512_mask_cmple_epu16_mask(~ascii_bitmask, utf16, v_07ff);
    __mmask32 not_one_two_bytes = ~(ascii_bitmask | two_bytes_bitmask);
    __mmask32 surrogates_bitmask = _mm512_mask_cmple_epu16_mask(not_one_two_bytes, utf16, v_dfff) & _mm512_mask_cmpge_epu16_mask(not_one_two_bytes, utf16, v_d800);

    size_t ascii_count = count_ones(ascii_bitmask);
    size_t two_bytes_count = count_ones(two_bytes_bitmask);
    size_t surrogate_bytes_count = count_ones(surrogates_bitmask);
    size_t three_bytes_count = 32 - ascii_count - two_bytes_count - surrogate_bytes_count;

    count += ascii_count + 2*two_bytes_count + 3*three_bytes_count + 2*surrogate_bytes_count;
  }

  return count + scalar::utf16::utf8_length_from_utf16<endianness::LITTLE>(ptr, length - (ptr - input));
}

simdutf_warn_unused size_t implementation::utf8_length_from_utf16be(const char16_t * input, size_t length) const noexcept {
  const char16_t* end = length >= 32 ? input + length - 32 : nullptr;
  const char16_t* ptr = input;

  const __m512i v_007f = _mm512_set1_epi16((uint16_t)0x007f);
  const __m512i v_07ff = _mm512_set1_epi16((uint16_t)0x07ff);
  const __m512i v_dfff = _mm512_set1_epi16((uint16_t)0xdfff);
  const __m512i v_d800 = _mm512_set1_epi16((uint16_t)0xd800);

  size_t count{0};
  const __m512i byteflip = _mm512_setr_epi64(
            0x0607040502030001,
            0x0e0f0c0d0a0b0809,
            0x0607040502030001,
            0x0e0f0c0d0a0b0809,
            0x0607040502030001,
            0x0e0f0c0d0a0b0809,
            0x0607040502030001,
            0x0e0f0c0d0a0b0809
        );
  while (ptr <= end) {
    __m512i utf16 = _mm512_loadu_si512((const __m512i*)ptr);
    utf16 = _mm512_shuffle_epi8(utf16, byteflip);
    ptr += 32;
    __mmask32 ascii_bitmask = _mm512_cmple_epu16_mask(utf16, v_007f);
    __mmask32 two_bytes_bitmask = _mm512_mask_cmple_epu16_mask(~ascii_bitmask, utf16, v_07ff);
    __mmask32 not_one_two_bytes = ~(ascii_bitmask | two_bytes_bitmask);
    __mmask32 surrogates_bitmask = _mm512_mask_cmple_epu16_mask(not_one_two_bytes, utf16, v_dfff) & _mm512_mask_cmpge_epu16_mask(not_one_two_bytes, utf16, v_d800);

    size_t ascii_count = count_ones(ascii_bitmask);
    size_t two_bytes_count = count_ones(two_bytes_bitmask);
    size_t surrogate_bytes_count = count_ones(surrogates_bitmask);
    size_t three_bytes_count = 32 - ascii_count - two_bytes_count - surrogate_bytes_count;
    count += ascii_count + 2*two_bytes_count + 3*three_bytes_count + 2*surrogate_bytes_count;
  }

  return count + scalar::utf16::utf8_length_from_utf16<endianness::BIG>(ptr, length - (ptr - input));
}

simdutf_warn_unused size_t implementation::utf32_length_from_utf16le(const char16_t * input, size_t length) const noexcept {
  return implementation::count_utf16le(input, length);
}

simdutf_warn_unused size_t implementation::utf32_length_from_utf16be(const char16_t * input, size_t length) const noexcept {
  return implementation::count_utf16be(input, length);
}

simdutf_warn_unused size_t implementation::utf16_length_from_latin1(size_t length) const noexcept {
  return scalar::latin1::utf16_length_from_latin1(length);
}


simdutf_warn_unused size_t implementation::utf32_length_from_latin1(size_t length) const noexcept {
  return scalar::latin1::utf32_length_from_latin1(length);
}

simdutf_warn_unused size_t implementation::utf8_length_from_latin1(const char * input, size_t length) const noexcept {
  const uint8_t *str = reinterpret_cast<const uint8_t *>(input);
  size_t answer = length / sizeof(__m512i) * sizeof(__m512i);
  size_t i = 0;
  char v_0xFF = static_cast<unsigned char>(0xFF);
  __m512i eight_64bits = _mm512_setzero_si512();
  while (i + sizeof(__m512i) <= length) {
    __m512i runner = _mm512_setzero_si512();
    size_t iterations = (length - i) / sizeof(__m512i);
    if (iterations > 255) {
      iterations = 255;
    }
    size_t max_i = i + iterations * sizeof(__m512i) - sizeof(__m512i);
    for (; i + 4*sizeof(__m512i) <= max_i; i += 4*sizeof(__m512i)) {
            // Load four __m512i vectors
            __m512i input1 = _mm512_loadu_si512((const __m512i *)(str + i));
            __m512i input2 = _mm512_loadu_si512((const __m512i *)(str + i + sizeof(__m512i)));
            __m512i input3 = _mm512_loadu_si512((const __m512i *)(str + i + 2*sizeof(__m512i)));
            __m512i input4 = _mm512_loadu_si512((const __m512i *)(str + i + 3*sizeof(__m512i)));

            // Generate four masks
            __mmask64 mask1 = _mm512_cmpgt_epi8_mask(_mm512_setzero_si512(), input1);
            __mmask64 mask2 = _mm512_cmpgt_epi8_mask(_mm512_setzero_si512(), input2);
            __mmask64 mask3 = _mm512_cmpgt_epi8_mask(_mm512_setzero_si512(), input3);
            __mmask64 mask4 = _mm512_cmpgt_epi8_mask(_mm512_setzero_si512(), input4);
            // Apply the masks and subtract from the runner
            __m512i not_ascii1 = _mm512_mask_set1_epi8(_mm512_setzero_si512(), mask1, v_0xFF);
            __m512i not_ascii2 = _mm512_mask_set1_epi8(_mm512_setzero_si512(), mask2, v_0xFF);
            __m512i not_ascii3 = _mm512_mask_set1_epi8(_mm512_setzero_si512(), mask3, v_0xFF);
            __m512i not_ascii4 = _mm512_mask_set1_epi8(_mm512_setzero_si512(), mask4, v_0xFF);

            runner = _mm512_sub_epi8(runner, not_ascii1);
            runner = _mm512_sub_epi8(runner, not_ascii2);
            runner = _mm512_sub_epi8(runner, not_ascii3);
            runner = _mm512_sub_epi8(runner, not_ascii4);
    }

    for (; i <= max_i; i += sizeof(__m512i)) {
      __m512i more_input = _mm512_loadu_si512((const __m512i *)(str + i));

      __mmask64 mask = _mm512_cmpgt_epi8_mask(_mm512_setzero_si512(), more_input);
      __m512i not_ascii = _mm512_mask_set1_epi8(_mm512_setzero_si512(), mask, v_0xFF);
      runner = _mm512_sub_epi8(runner, not_ascii);
    }

    eight_64bits = _mm512_add_epi64(eight_64bits, _mm512_sad_epu8(runner, _mm512_setzero_si512()));
  }

  __m256i first_half = _mm512_extracti64x4_epi64(eight_64bits, 0);
  __m256i second_half = _mm512_extracti64x4_epi64(eight_64bits, 1);
  answer += (size_t)_mm256_extract_epi64(first_half, 0) +
            (size_t)_mm256_extract_epi64(first_half, 1) +
            (size_t)_mm256_extract_epi64(first_half, 2) +
            (size_t)_mm256_extract_epi64(first_half, 3) +
            (size_t)_mm256_extract_epi64(second_half, 0) +
            (size_t)_mm256_extract_epi64(second_half, 1) +
            (size_t)_mm256_extract_epi64(second_half, 2) +
            (size_t)_mm256_extract_epi64(second_half, 3);
  return answer + scalar::latin1::utf8_length_from_latin1(reinterpret_cast<const char *>(str + i), length - i);
}

simdutf_warn_unused size_t implementation::utf16_length_from_utf8(const char * input, size_t length) const noexcept {
    size_t pos = 0;
    size_t count = 0;
    // This algorithm could no doubt be improved!
    for(;pos + 64 <= length; pos += 64) {
      __m512i utf8 = _mm512_loadu_si512((const __m512i*)(input+pos));
      uint64_t utf8_continuation_mask = _mm512_cmple_epi8_mask(utf8, _mm512_set1_epi8(-65+1));
      // We count one word for anything that is not a continuation (so
      // leading bytes).
      count += 64 - count_ones(utf8_continuation_mask);
      uint64_t utf8_4byte = _mm512_cmpge_epu8_mask(utf8, _mm512_set1_epi8(int8_t(240)));
      count += count_ones(utf8_4byte);
    }
    return count + scalar::utf8::utf16_length_from_utf8(input + pos, length - pos);
}

simdutf_warn_unused size_t implementation::utf8_length_from_utf32(const char32_t * input, size_t length) const noexcept {
  const char32_t* end = length >= 16 ? input + length - 16 : nullptr;
  const char32_t* ptr = input;

  const __m512i v_0000_007f = _mm512_set1_epi32((uint32_t)0x7f);
  const __m512i v_0000_07ff = _mm512_set1_epi32((uint32_t)0x7ff);
  const __m512i v_0000_ffff = _mm512_set1_epi32((uint32_t)0x0000ffff);

  size_t count{0};

  while (ptr <= end) {
    __m512i utf32 = _mm512_loadu_si512((const __m512i*)ptr);
    ptr += 16;
    __mmask16 ascii_bitmask = _mm512_cmple_epu32_mask(utf32, v_0000_007f);
    __mmask16 two_bytes_bitmask = _mm512_mask_cmple_epu32_mask(_knot_mask16(ascii_bitmask), utf32, v_0000_07ff);
    __mmask16 three_bytes_bitmask = _mm512_mask_cmple_epu32_mask(_knot_mask16(_mm512_kor(ascii_bitmask, two_bytes_bitmask)), utf32, v_0000_ffff);

    size_t ascii_count = count_ones(ascii_bitmask);
    size_t two_bytes_count = count_ones(two_bytes_bitmask);
    size_t three_bytes_count = count_ones(three_bytes_bitmask);
    size_t four_bytes_count = 16 - ascii_count - two_bytes_count - three_bytes_count;
    count += ascii_count + 2*two_bytes_count + 3*three_bytes_count + 4*four_bytes_count;
  }

  return count + scalar::utf32::utf8_length_from_utf32(ptr, length - (ptr - input));
}

simdutf_warn_unused size_t implementation::utf16_length_from_utf32(const char32_t * input, size_t length) const noexcept {
  const char32_t* end = length >= 16 ? input + length - 16 : nullptr;
  const char32_t* ptr = input;

  const __m512i v_0000_ffff = _mm512_set1_epi32((uint32_t)0x0000ffff);

  size_t count{0};

  while (ptr <= end) {
    __m512i utf32 = _mm512_loadu_si512((const __m512i*)ptr);
    ptr += 16;
    __mmask16 surrogates_bitmask = _mm512_cmpgt_epu32_mask(utf32, v_0000_ffff);

    count += 16 + count_ones(surrogates_bitmask);
  }

  return count + scalar::utf32::utf16_length_from_utf32(ptr, length - (ptr - input));
}

simdutf_warn_unused size_t implementation::utf32_length_from_utf8(const char * input, size_t length) const noexcept {
  return implementation::count_utf8(input, length);
}

} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf

#include "simdutf/icelake/end.h"
