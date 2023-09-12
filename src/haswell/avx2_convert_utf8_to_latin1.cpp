// depends on "tables/utf8_to_utf16_tables.h"

// for reference, this is the previous version:

// scalar:
/* convert_utf8_to_latin1+haswell, input size: 440052, iterations: 3000, dataset: unicode_lipsum/wikipedia_mars/french.utflatin8.txt   
  5.281 ins/byte,    1.872 cycle/byte,    1.706 GB/s (50.2 %),     3.193 GHz,    2.821 ins/cycle 
   5.376 ins/char,    1.906 cycle/char,    1.676 Gc/s (50.2 %)     1.02 byte/char  */

// SSE
/* convert_utf8_to_latin1+haswell, input size: 440052, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utflatin8.txt
   1.760 ins/byte,    0.970 cycle/byte,    3.292 GB/s (2.2 %),     3.194 GHz,    1.814 ins/cycle 
   1.792 ins/char,    0.988 cycle/char,    3.234 Gc/s (2.2 %)     1.02 byte/char  */

// par
/* convert_utf8_to_latin1+haswell, input size: 440052, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utflatin8.txt
   1.760 ins/byte,    0.968 cycle/byte,    3.300 GB/s (2.3 %),     3.194 GHz,    1.819 ins/cycle 
   1.792 ins/char,    0.985 cycle/char,    3.242 Gc/s (2.3 %)     1.02 byte/char  */

   /* Unrolled version:
convert_utf8_to_latin1+haswell, input size: 440052, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utflatin8.txt
The output is zero which might indicate an error.
   1.705 ins/byte,    0.974 cycle/byte,    3.278 GB/s (2.7 %),     3.194 GHz,    1.750 ins/cycle 
   1.735 ins/char,    0.992 cycle/char,    3.220 Gc/s (2.7 %)     1.02 byte/char   */

// Convert up to 12 bytes from utf8 to latin1 using a mask indicating the
// end of the code points. Only the least significant 12 bits of the mask
// are accessed.
// It returns how many bytes were consumed (up to 12).
size_t convert_masked_utf8_to_latin1(const char *input,
                           uint64_t utf8_end_of_code_point_mask,
                           char *&latin1_output) {
  // we use an approach where we try to process up to 12 input bytes.
  // Why 12 input bytes and not 16? Because we are concerned with the size of
  // the lookup tables. Also 12 is nicely divisible by two and three.
  //
  //
  // Optimization note: our main path below is load-latency dependent. Thus it is maybe
  // beneficial to have fast paths that depend on branch prediction but have less latency.
  // This results in more instructions but, potentially, also higher speeds.
  //
  // const __m128i in = _mm_loadu_si128((__m128i *)input);

  // Load 32 bytes from 'input' into a 256-bit AVX2 register
const __m256i in_combined = _mm256_loadu_si256((__m256i*)input);

// Extract the lower and higher 128-bits into __m128i variables
const __m128i in = _mm256_extracti128_si256(in_combined, 0); // Lower 128 bits
const __m128i in_second_half = _mm256_extracti128_si256(in_combined, 1); // Higher 128 bits


  const uint16_t input_utf8_end_of_code_point_mask =
      utf8_end_of_code_point_mask & 0xfff; //we're only processing 12 bytes in case it`s not all ASCII
  const uint16_t input_utf8_end_of_code_point_mask_2 =
    (utf8_end_of_code_point_mask & 0xfff000) >> 12; //we're only processing 12 bytes in case it`s not all ASCII

/*   if((input_utf8_end_of_code_point_mask & 0xffffffff) == 0xffffffff) {
    // Load the next 128 bits.

    // Combine the two 128-bit registers into a single 256-bit register.
    __m256i in_combined = _mm256_set_m128i(in_second_half, in);

    // We process the data in chunks of 32 bytes.
    _mm256_storeu_si256(reinterpret_cast<__m256i *>(latin1_output), in_combined);

    latin1_output += 32; // We wrote 32 characters.
    return 32; // We consumed 32 bytes.
  } */

  if(((utf8_end_of_code_point_mask & 0xffff) == 0xffff)) {
    // We process the data in chunks of 16 bytes.
    _mm_storeu_si128(reinterpret_cast<__m128i *>(latin1_output), in);
    latin1_output += 16; // We wrote 16 characters.
    return 16; // We consumed 16 bytes.
  }
  /// We do not have a fast path available, so we fallback.
  const uint8_t idx =
      tables::utf8_to_utf16::utf8bigindex[input_utf8_end_of_code_point_mask][0];
  const uint8_t consumed =
      tables::utf8_to_utf16::utf8bigindex[input_utf8_end_of_code_point_mask][1];

  if(idx >= 64) { return consumed; }


  // const __m128i in_second_half = _mm_loadu_si128((__m128i *)(input + consumed));
  
  const uint8_t idx2 =
      tables::utf8_to_utf16::utf8bigindex[input_utf8_end_of_code_point_mask_2][0];
  const uint8_t consumed2 =
      tables::utf8_to_utf16::utf8bigindex[input_utf8_end_of_code_point_mask_2][1];

  // this indicates an invalid input:
  // if(idx >= 64) { return consumed; }

  if(idx2 >=64) { return consumed + consumed2; } //This causes a performance hit


  // Here we should have (idx < 64), if not, there is a bug in the validation or elsewhere.
  // SIX (6) input code-words
  // this is a relatively easy scenario
  // we process SIX (6) input code-words. The max length in bytes of six code
  // words spanning between 1 and 2 bytes each is 12 bytes. On processors
  // where pdep/pext is fast, we might be able to use a small lookup table.
/*   const __m128i sh =
        _mm_loadu_si128((const __m128i *)tables::utf8_to_utf16::shufutf8[idx]);
  const __m128i perm = _mm_shuffle_epi8(in, sh);
  const __m128i ascii = _mm_and_si128(perm, _mm_set1_epi16(0x7f));
  const __m128i highbyte = _mm_and_si128(perm, _mm_set1_epi16(0x1f00));
  __m128i composed = _mm_or_si128(ascii, _mm_srli_epi16(highbyte, 2));
  const __m128i latin1_packed = _mm_packus_epi16(composed,composed);
 */

/* auto perform_operations = [&]() -> __m128i {
  const __m128i sh =
        _mm_loadu_si128((const __m128i *)tables::utf8_to_utf16::shufutf8[idx]);
  const __m128i perm = _mm_shuffle_epi8(in, sh);
  const __m128i ascii = _mm_and_si128(perm, _mm_set1_epi16(0x7f));
  const __m128i highbyte = _mm_and_si128(perm, _mm_set1_epi16(0x1f00));
  __m128i composed = _mm_or_si128(ascii, _mm_srli_epi16(highbyte, 2));
  return _mm_packus_epi16(composed, composed);
}; */

auto perform_operations = [&](uint8_t idx) -> __m128i {
  const __m128i sh =
        _mm_loadu_si128((const __m128i *)tables::utf8_to_utf16::shufutf8[idx]);
  const __m128i perm = _mm_shuffle_epi8(in, sh); // "splits" the utf-8 units so that they all take two bytes. 
  // e.g. say X is ASCII, Y are non-ASCII header bytes, and Z are continuation bytes, and 0 represents zero byte then we'll have:
  // XYZXXXYZ => X 0 Y Z X 0 X 0 Y Z
  const __m128i ascii = _mm_and_si128(perm, _mm_set1_epi16(0x7f)); // identify which byte is ASCII
  const __m128i highbyte = _mm_and_si128(perm, _mm_set1_epi16(0x1f00)); // identify which byte is high byte 0001 1111
  __m128i composed = _mm_or_si128(ascii,  //put ascii
                                  _mm_srli_epi16(highbyte, 2)); // and high bytes shifted by two bits together. 
//   return _mm_packus_epi16(composed, composed);
    return composed;
};


__m128i result1 = perform_operations(idx);
__m128i result2 = perform_operations(idx2);

__m128i latin1_packed = _mm_packus_epi16(result1, result2);


/* auto handle_fallback = [&](uint16_t input_utf8_end_of_code_point_mask_lambda) -> std::pair<__m128i, uint8_t> {
  const uint8_t idx =
      tables::utf8_to_utf16::utf8bigindex[input_utf8_end_of_code_point_mask_lambda][0];
  const uint8_t consumed =
      tables::utf8_to_utf16::utf8bigindex[input_utf8_end_of_code_point_mask_lambda][1];

  if(idx >= 64) {
    return {_mm_setzero_si128(), consumed};
  }

  const __m128i sh = _mm_loadu_si128((const __m128i *)tables::utf8_to_utf16::shufutf8[idx]);
  const __m128i perm = _mm_shuffle_epi8(in, sh);
  const __m128i ascii = _mm_and_si128(perm, _mm_set1_epi16(0x7f));
  const __m128i highbyte = _mm_and_si128(perm, _mm_set1_epi16(0x1f00));
  __m128i composed = _mm_or_si128(ascii, _mm_srli_epi16(highbyte, 2));
  const __m128i latin1_packed = _mm_packus_epi16(composed, composed);

  return {latin1_packed, consumed};
}; 

std::pair<__m128i, uint8_t> result = handle_fallback(input_utf8_end_of_code_point_mask);
const __m128i latin1_packed = result.first;
const uint8_t consumed = result.second;*/



  // writing 8 bytes even though we only care about the first 6 bytes.
  // performance note: it would be faster to use _mm_storeu_si128, we should investigate.
  // _mm_storel_epi64((__m128i *)latin1_output, latin1_packed);
//   _mm256_storeu_si256((__m256i*)latin1_output, latin1_packed);
  _mm_storeu_si128((__m128i *)latin1_output, latin1_packed);

  latin1_output += 12; // We wrote 6 bytes.
  return consumed + consumed2;
}


//  ----------------------------------------

// file included directly

// File contains conversion procedure from possibly invalid UTF-8 strings.

template <bool is_remaining>
size_t process_block_avx2(const char *buf, size_t len, char *latin_output,
                          __m256i minus64, __m256i one,
                          uint32_t *next_leading_ptr, uint32_t *next_bit6_ptr) {
    uint32_t load_mask = is_remaining ? (1U << len) - 1 : ~0U;
    
    // Masked load using bitwise operations
    __m256i input = _mm256_and_si256(
                                    _mm256_loadu_si256(
                                                        (__m256i *)buf), 
                                                        _mm256_set1_epi8(load_mask));
    __m256i nonascii = _mm256_cmpgt_epi8(
                                        input,
                                         _mm256_setzero_si256());
    
    if (_mm256_testz_si256(nonascii, nonascii)) {
/*         _mm256_storeu_si256((__m256i *)latin_output, input);
        return len; */
        if(is_remaining) {
            //Placeholder
/*             // Read the existing data in the destination
            __m256i existing_data = _mm256_loadu_si256((__m256i*)buf);//latin_output);

            // Prepare a blend mask from load_mask 
            __m256i blend_mask = _mm256_set1_epi32(load_mask); 

            // Blend the existing data with your input data based on the mask
            __m256i blended_data = _mm256_blendv_epi8(existing_data, input, blend_mask);

            // Store the result
            _mm256_storeu_si256((__m256i*)latin_output, blended_data); */
        }
        else {
            // No mask, just store everything
            _mm256_storeu_si256((__m256i*)latin_output, input);
        }

    }
    
    // __m256i leading = _mm256_cmpgt_epi8(input, minus64);
    __m256i leading = _mm256_cmpgt_epi8( // likely error <- the OG verison has epu and not epi
                                        input, 
                                        _mm256_sub_epi8(
                                                        minus64, 
                                                        _mm256_set1_epi8(1)));
    __m256i highbits = _mm256_xor_si256(
                                        input, 
                                        _mm256_set1_epi8(-62)); // 1100 0010


    __m256i invalid_leading_bytes = _mm256_and_si256(
                                                        leading, 
                                                        _mm256_cmpgt_epi8(
                                                                            highbits,
                                                                            one));
    
    // Replace this with actual mask comparison logic as needed
    if (_mm256_testz_si256(invalid_leading_bytes, invalid_leading_bytes)) {
        return 0;  // Indicates error
    }

// ----------------------------------------                                                        

    // This part is tricky since AVX2 doesn't support mask shifts directly
    // uint32_t leading_shift = (*next_leading_ptr << 1) | (*next_leading_ptr >> 31);
    uint32_t leading_shift = (leading << 1) | *next_leading_ptr;//(*next_leading_ptr >> 31);
    *next_leading_ptr = *next_leading_ptr >> 31;
    
if ((nonascii ^ leading) != leading_shift) {
  return 0; // Indicates error
}


/* 
// Assuming that highbits, one, input, and minus64 are __m256i variables
__m256i cmp_mask = _mm256_cmpeq_epi8(highbits, one);  // Compare
int mask = _mm256_movemask_epi8(cmp_mask);  // Move to integer mask
mask = (mask << 1) | (*next_bit6_ptr);  // Shift and merge with next_bit6_ptr

// Create a full __m256i mask from the integer mask
__m256i full_mask = _mm256_set1_epi8(mask);

// Do subtraction where the mask is true
__m256i to_subtract = _mm256_and_si256(full_mask, minus64);
input = _mm256_sub_epi8(input, to_subtract);

// Update next_bit6_ptr for the next round
*next_bit6_ptr = mask >> 31;  // Shift down the last bit */

__m256i bit6 = _mm256_cmpeq_epi8(highbits, one); // Compare highbits and one

// Create a mask from the shifted bit6 and the stored next_bit6 value
__m256i next_bit6_vector = _mm256_set1_epi8(*next_bit6_ptr);
__m256i shifted_bit6 = _mm256_slli_si256(bit6, 1); // Shift left by one byte
__m256i mask = _mm256_or_si256(shifted_bit6, next_bit6_vector);

// Perform the masked subtraction
__m256i subtract_mask = _mm256_and_si256(mask, minus64);
input = _mm256_sub_epi8(input, subtract_mask);

// Update next_bit6_ptr for the next round
*next_bit6_ptr = _mm256_extract_epi8(bit6, 31); // Extract the last byte



// Compute retain and output: Assuming load_mask is an int32_t
__m256i leading_not = _mm256_xor_si256(leading, _mm256_set1_epi8(-1));
__m256i retain = _mm256_and_si256(leading_not, _mm256_set1_epi8(load_mask));
// AVX2 does not have a simple mask-based compression instruction like _mm512_maskz_compress_epi8.
//  implement the compression operation 

int64_t written_out = count_ones(retain);

// Store mask
__m256i store_mask = _mm256_set1_epi8((1ULL << written_out) - 1);
// Again, AVX2 lacks an equivalent mask-store instruction
    output = _mm256_and_si256(output, _mm256_set1_epi8(written_out));
    _mm256_storeu_si256((__m256i *)latin_output, output);

    return written_out;
}

size_t utf8_to_latin1_avx2(const char *buf, size_t len, char *latin_output) {
    char *start = latin_output;
    size_t pos = 0;
    __m256i minus64 = _mm256_set1_epi8(-64); // 11111111111 ... 1100 0000
    __m256i one = _mm256_set1_epi8(1);
    unsigned int next_leading = 0; // Changed to integer as AVX2 doesn't have masks like AVX512
    unsigned int next_bit6 = 0; // Changed to integer as AVX2 doesn't have masks like AVX512

    while (pos + 32 <= len) { // Changed loop increment from 64 to 32 to fit 256-bit vector
        size_t written = process_block_avx2<false>(buf + pos, 32, latin_output, minus64, one, &next_leading, &next_bit6);
        if (written == 0) {
            return 0; // Indicates error
        }
        latin_output += written;
        pos += 32; // Increment by 32 bytes to fit 256-bit vector
    }

    if (pos < len) {
        size_t remaining = len - pos;
        size_t written = process_block_avx2<true>(buf + pos, remaining, latin_output, minus64, one, &next_leading, &next_bit6);
        if (written == 0) {
            return 0; // Indicates error
        }
        latin_output += written;
    }

    return (size_t)(latin_output - start);
}



