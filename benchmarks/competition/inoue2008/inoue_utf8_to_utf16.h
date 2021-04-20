/**
 * We do a full header implementation for simplicity. This code is for
 * comparison purposes only and not meant to be used in production (ever).
 * 
 * Not to be released as part of our public API. This is private code.
 */

// This is an implementation of  Accelerating UTF-8 Decoding Using SIMD
// Instructions based on Hiroshi Inoue and Hideaki Komatsu and Toshio Nakatani,
// Accelerating UTF-8 Decoding Using SIMD Instructions (in Japanese),
// Information Processing Society of Japan Transactions on Programming 1 (2),
// 2008. Slides at
// https://researcher.watson.ibm.com/researcher/files/jp-INOUEHRS/IPSJPRO2008_SIMDdecoding.pdf
//
// It appears that the original implementation was done for IBM hardware. We do
// not know of any public implementation.
//
// It does not handle 4-byte inputs.
// It does not validate the input.

/***
 * Implementation note:
 *  Inoue et al. rely on POWER's vector_permute intrinsic which can take two
 * 16-byte registers, and an additional 16-byte index and then "permute" or
 * "shuffle" them (across lanes). Under x64, there are fast instructions for
 * this purpose starting with AVX-512 ISAs but, unfortunately, nothing of the
 * sort for SSE* and AVX2. You can do two 128-bit shuffle at once with AVX2, but
 * you cannot cross lanes.
 *
 * This is problematic for x64 (pre AVX-512). However, NEON has the appropriate
 * instructions.
 */
#ifdef __ARM_NEON
#ifndef INOUE_UTF8_TO_UTF16_H
#define INOUE_UTF8_TO_UTF16_H
#define INOUE2008
#include <arm_neon.h>
#include "inoue_utf8_to_utf16_tables.h"
namespace inoue2008 {

// Same type (i.e. same length in UTF-8 representation) of characters tends to
// appear repeatedly in real-world data

static inline uint8x16x2_t vector_load_32bytes(const uint8_t *ptr) noexcept {
  return vld2q_u8(ptr);
}
static inline uint8x16_t vector_load_16bytes(const uint8_t *ptr) noexcept {
  return vld1q_u8(ptr);
}
static inline void vector_store(char16_t *ptr, uint16x8_t a) noexcept {
  return vst1q_u16((uint16_t *)ptr, a);
}

static inline uint16x8_t vector_constant_u16(uint16_t c) noexcept {
  return vmovq_n_u16(c);
}

static inline uint16x8_t vector_constant_u8(uint8_t c) noexcept {
  return vmovq_n_u8(c);
}

// emulate the POWER vec_perm intrinsic.
static inline uint16x8_t vector_permute(uint8x16x2_t a,
                                        uint8x16_t shuf) noexcept {
  return vqtbl2q_u8(a, shuf);
}

static inline uint16x8_t vector_select(uint16x8_t a, uint16x8_t b,
                                       uint16x8_t c) noexcept {
  return vbslq_u16(a, b, c);
}

static inline uint16x8_t vector_and(uint16x8_t a, uint16x8_t b) noexcept {
  return vandq_u16(a, b);
}

static inline uint16x8_t vector_or(uint16x8_t a, uint16x8_t b) noexcept {
  return vorrq_u16(a, b);
}

template <int n>
static inline uint16x8_t vector_shift_left(uint16x8_t a) noexcept {
  return vshlq_n_u16(a, n);
}


static inline size_t scalar_convert_valid(const char* buf, size_t len, char16_t* utf16_output) {
 const uint8_t *data = reinterpret_cast<const uint8_t *>(buf);
  size_t pos = 0;
  char16_t* start{utf16_output};
  while (pos < len) {
    // try to convert the next block of 8 ASCII bytes
    if (pos + 8 <= len) { // if it is safe to read 8 more bytes, check that they are ascii
      uint64_t v;
      ::memcpy(&v, data + pos, sizeof(uint64_t));
      if ((v & 0x8080808080808080) == 0) {
        size_t final_pos = pos + 8;
        while(pos < final_pos) {
          *utf16_output++ = char16_t(buf[pos]);
          pos++;
        }
        continue;
      }
    }
    uint8_t leading_byte = data[pos]; // leading byte
    printf("leading byte %u\n", leading_byte);
    if (leading_byte < 0b10000000) {
      // converting one ASCII byte !!!
      printf("ASCII %c\n", leading_byte);
      *utf16_output++ = char16_t(leading_byte);
      pos++;
    } else if ((leading_byte & 0b11100000) == 0b11000000) {
      // We have a two-byte UTF-8, it should become
      // a single UTF-16 word.
      if(pos + 1 > len) { break; } // minimal bound checking
      *utf16_output++ = char16_t(((leading_byte &0b00011111) << 6) | (data[pos + 1] &0b00111111));
      pos += 2;
    } else if ((leading_byte & 0b11110000) == 0b11100000) {
      // We have a three-byte UTF-8, it should become
      // a single UTF-16 word.
      if(pos + 2 > len) { break; } // minimal bound checking
      *utf16_output++ = char16_t(((leading_byte &0b00001111) << 12) | ((data[pos + 1] &0b00111111) << 6) | (data[pos + 2] &0b00111111));
      pos += 3;
    } else if ((leading_byte & 0b11111000) == 0b11110000) { // 0b11110000
      // we have a 4-byte UTF-8 word.
      if(pos + 3 > len) { break; } // minimal bound checking
      uint32_t code_word = ((leading_byte & 0b00000111) << 18 )| ((data[pos + 1] &0b00111111) << 12)
                           | ((data[pos + 2] &0b00111111) << 6) | (data[pos + 3] &0b00111111);
      code_word -= 0x10000;
      *utf16_output++ = char16_t(0xD800 + (code_word >> 10));
      *utf16_output++ = char16_t(0xDC00 + (code_word & 0x3FF));
      pos += 4;
    } else {
      // we may have a continuation but we do not do error checking
    }
  }
  return utf16_output - start;
}


// Input should be UTF-8 with 1, 2 or 3 -byte characters.
static inline size_t convert_valid(const char *input_char, size_t size,
                     char16_t *utf16_output) noexcept {
  char16_t *utf16_output_orig = utf16_output;
  size_t position{0};
  // We should have
  //   const static uint8_t prefix_to_length_table[] = {1, 1, 1, 1, -, -, 2, 3};
  // But we need fill something in for the continuation byte. Setting 0 might be disastrous.
  // So let us use 1.
  const static uint8_t prefix_to_length_table[] = {1, 1, 1, 1, 1, 1, 2, 3};
  const uint8_t *input = reinterpret_cast<const uint8_t *>(input_char);
  // The algorithm may read up to 32 bytes forward.
  while (position + 32 <= size) {
    uint32_t gathered_prefix{0};
    // step 1: gather prefix of 8 characters and convert them to length in bytes
    // This covers up to 24 bytes.
    const uint8_t * const p  = input;
    for (int i = 0; i < 8; i++) {
      // The original paper takes (input[position] >> 3) which leaves 5 bits out of 8 bits.
      // That makes little sense unless the algorithm does validation but it clearly does
      // not since it jumps from leading bytes to leading bytes.
      const uint8_t prefix = (input[position] >> 5);
      // It is not clear that a table is needed, but let us stick with the original 
      // algorithm.
      const uint8_t length = prefix_to_length_table[prefix];       
      // If length == 0, then we hit a continuation and the whole thing is garbage.
      //
      // The original paper has gathered_prefix = (gathered_prefix * 3) + length;
      // but this makes no sense since length is in [1,3]. It does make sense if
      // we map length to 0,1,2.
      printf("length = %d", length);
      gathered_prefix = (gathered_prefix * 3) + (length - 1);
      position += length;
    }
    // gathered_prefix < 6561
    // step 2: load constants from tables
    const auto vpattern1 = vector_load_16bytes(pattern1[gathered_prefix]);
    const auto vpattern2 = vector_load_16bytes(pattern2[gathered_prefix]);
    //
    // We simplified the algorithm.
    //
    // const auto vmask1 = mask_for_select_table[gathered_prefix];
    // const auto vmask2 = mask_for_and_table[gathered_prefix];

    // step 3: move data bits using constants

    // We take 32 bytes
    const auto v = vector_load_32bytes(p);
    for(size_t i = 0; i < 32;i++) {
        printf("%x ", p[i]);
    }
    printf("\n");
    printf("vpattern1\n");
    for(size_t i = 0; i < 16;i++) {
        printf("%x ", vpattern1[i]);
    }
    printf("\n");
    printf("vpattern2\n");

        for(size_t i = 0; i < 16;i++) {
        printf("%x ", vpattern2[i]);
    }
    printf("\n");
    auto vtmp1 = vector_permute(v, vpattern1);
    for(size_t i = 0; i < 8;i++) {
        printf("%x ", vtmp1[i]);
    }
    printf("\n");
    auto vtmp2 = vector_permute(v, vpattern2);
    for(size_t i = 0; i < 8;i++) {
        printf("%x ", vtmp2[i]);
    }
    printf("\n");
    // Original paper has :
    // const auto vconstant_0x0FC0 = vector_constant_u16(0x0FC0);
    // But it does not make good sense.
    //
    // vtmp1 contains up to 10 bits
    // 1110ABCD 10EFGHIJ (10 bits)
    // or 
    // 110ABCDE (5 bits)
    //
    // Shift left by four
    // ABCD10EF GHIJ----
    // or
    //     110A BCDE----  
    //
    // Shift left by six
    // CD10EFGH IJ------
    // or
    //   110ABC DE------
    //
    // We select with 
    //   0b1111 11111111
    // We get either
    // ABCDEFGH IJ------
    // or
    // ----0ABC DE------
    vtmp1 = vector_select(vector_shift_left<4>(vtmp1),
                               vector_shift_left<6>(vtmp1), vector_constant_u16(0x0FFF));
    // The original algorithm seemed to complicated so I simplified it.
    //
    // If vtmp2 contains the 'least' significant word, then we just
    // need to mask its most significant bits and to "OR".
    vtmp2 = vector_and(vtmp2, vector_constant_u8(0x7F));
   // const auto vout = vector_or(vtmp2, vtmp1);
    // The original algorithm was as follow:
    // const auto vtmp3 = vector_select(vtmp1, vtmp2, vmask1);
    // step 4: mask off unused bits
    // const auto vout = vector_and(vtmp3, vmask2);
    // So it required two additional masks!!!
    // step 5: write out the result
   // vector_store(utf16_output, vout);
    vector_store(utf16_output, vtmp2);
    utf16_output += 8; // We always write 8 characters.
  }
  printf("position = %zu \n", position);
    printf("size - position = %zu \n", size - position);
  // Finish the tail.
  size_t tail_length = scalar_convert_valid(input_char + position, size - position, utf16_output);
  return utf16_output - utf16_output_orig + tail_length;
}



// minimal testing
static inline void inoue_test() {
    char16_t utf16_output[50];
    size_t len = inoue2008::convert_valid("abcd", 4,utf16_output);
   // len = 4
//61 62 63 64
    printf("len = %zu\n", len);
    for(size_t i = 0; i < len;i++) {
        printf("%x ", utf16_output[i]);
    }
    printf("\n");
   len = inoue2008::convert_valid("\xc3\xa9\x74\xc3\xa9", 5,utf16_output);
   //len = 3
   //char16_t expected2 = {0xe9, 0x74 0xe9};
//e9 74 e9
    printf("len = %zu\n", len);
    for(size_t i = 0; i < len;i++) {
        printf("%x ", utf16_output[i]);
    }
    printf("\n");
    len = inoue2008::convert_valid("\xe9\xac\xb2\x20\xe9\xac\xbc                                   ", 41,utf16_output);
    //f(len != 3) {
     //   throw std::runtime_error("bug");
    //}
    // 9b32 20 9b3c 20 20 20 20 20 20 20 20 20 20 20 20 20 20 20 20 20 20 20 20 20 20 20 20
    //char16_t expected3 = {0x9b32, 0x20 0x9b3c};
    printf("len = %zu\n", len);
    for(size_t i = 0; i < len;i++) {
        printf("%x ", utf16_output[i]);
    }
   // len = 3

    printf("\n");    
}
} // namespace inoue2008
#endif 
#endif
