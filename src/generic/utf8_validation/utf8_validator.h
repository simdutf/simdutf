namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {
namespace utf8_validation {

/**
 * Validates that the string is actual UTF-8.
 */
template<class checker>
bool generic_validate_utf8(const uint8_t * input, size_t length) {
    checker c{};
    buf_block_reader<64> reader(input, length);
    while (reader.has_full_block()) {
      simd::simd8x64<uint8_t> in(reader.full_block());
      c.check_next_input(in);
      reader.advance();
    }
    uint8_t block[64]{};
    reader.get_remainder(block);
    simd::simd8x64<uint8_t> in(block);
    c.check_next_input(in);
    reader.advance();
    c.check_eof();
    return !c.errors();
}

bool generic_validate_utf8(const char * input, size_t length) {
    return generic_validate_utf8<utf8_checker>(reinterpret_cast<const uint8_t *>(input),length);
}

template<class checker>
bool generic_validate_ascii(const uint8_t * input, size_t length) {
    buf_block_reader<64> reader(input, length);
    uint8_t blocks[64]{};
    simd::simd8x64<uint8_t> running_or(blocks);
    while (reader.has_full_block()) {
      simd::simd8x64<uint8_t> in(reader.full_block());
      running_or |= in;
      reader.advance();
    }
    uint8_t block[64]{};
    reader.get_remainder(block);
    simd::simd8x64<uint8_t> in(block);
    running_or |= in;
    reader.advance();
    return running_or.is_ascii();
}

bool generic_validate_ascii(const char * input, size_t length) {
    return generic_validate_ascii<utf8_checker>(reinterpret_cast<const uint8_t *>(input),length);
}

} // namespace utf8_validation
} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf
