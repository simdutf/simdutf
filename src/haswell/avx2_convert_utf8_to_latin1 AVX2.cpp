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


