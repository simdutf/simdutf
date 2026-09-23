/**
 * References and further reading:
 *
 * Wojciech Muła, Daniel Lemire, Base64 encoding and decoding at almost the
 * speed of a memory copy, Software: Practice and Experience 50 (2), 2020.
 * https://arxiv.org/abs/1910.05109
 *
 * Wojciech Muła, Daniel Lemire, Faster Base64 Encoding and Decoding using AVX2
 * Instructions, ACM Transactions on the Web 12 (3), 2018.
 * https://arxiv.org/abs/1704.00605
 *
 * Simon Josefsson. 2006. The Base16, Base32, and Base64 Data Encodings.
 * https://tools.ietf.org/html/rfc4648. (2006). Internet Engineering Task Force,
 * Request for Comments: 4648.
 *
 * Alfred Klomp. 2014a. Fast Base64 encoding/decoding with SSE vectorization.
 * http://www.alfredklomp.com/programming/sse-base64/. (2014).
 *
 * Alfred Klomp. 2014b. Fast Base64 stream encoder/decoder in C99, with SIMD
 * acceleration. https://github.com/aklomp/base64. (2014).
 *
 * Hanson Char. 2014. A Fast and Correct Base 64 Codec. (2014).
 * https://aws.amazon.com/blogs/developer/a-fast-and-correct-base-64-codec/
 *
 * Nick Kopp. 2013. Base64 Encoding on a GPU.
 * https://www.codeproject.com/Articles/276993/Base-Encoding-on-a-GPU. (2013).
 */
namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {
namespace base64 {

/*
    The following template function implements API for Base64 decoding.

    An implementation is responsible for providing the `block64` type and
    associated methods that perform actual conversion. Please refer
    to any vectorized implementation to learn the API of these procedures.
*/
template <bool base64_url, bool ignore_garbage, bool default_or_url,
          typename chartype>
full_result
compress_decode_base64(char *dst, const chartype *src, size_t srclen,
                       base64_options options,
                       last_chunk_handling_options last_chunk_options) {
  const uint8_t *to_base64 =
      default_or_url ? tables::base64::to_base64_default_or_url_value
                     : (base64_url ? tables::base64::to_base64_url_value
                                   : tables::base64::to_base64_value);
  auto ri = simdutf::scalar::base64::find_end(src, srclen, options);
  size_t equallocation = ri.equallocation;
  size_t equalsigns = ri.equalsigns;
  srclen = ri.srclen;
  size_t full_input_length = ri.full_input_length;
  if (srclen == 0) {
    if (!ignore_garbage && equalsigns > 0) {
      return {INVALID_BASE64_CHARACTER, equallocation, 0, true};
    }
    return {SUCCESS, full_input_length, 0};
  }
  const chartype *const srcinit = src;
  const char *const dstinit = dst;
  const chartype *const srcend = src + srclen;

  constexpr size_t block_size = 6;
  static_assert(block_size >= 2, "block_size must be at least two");
  char buffer[block_size * 64];
  char *bufferptr = buffer;
  // A wide block store writes 4 bytes past its 48. That is safe when the next
  // store starts 48 bytes later. Pairs of clean blocks wide-store the first
  // and end the second on byte 48. Anything shorter, or a block that is not
  // clean, uses the exact store. The pair attempt is only worth it on a long
  // input: a short one often fails the first block and then repeats that work.
  if (srclen >= 512) {
    const chartype *const srcend128 = src + srclen - 128;
    while (bufferptr == buffer && src <= srcend128) {
      block64 b0(src);
      uint64_t e0 = 0;
      const uint64_t m0 =
          b0.to_base64_mask<base64_url, ignore_garbage, default_or_url>(&e0);
      if ((!ignore_garbage && e0) || m0 != 0) {
        break;
      }
      block64 b1(src + 64);
      uint64_t e1 = 0;
      const uint64_t m1 =
          b1.to_base64_mask<base64_url, ignore_garbage, default_or_url>(&e1);
      if ((!ignore_garbage && e1) || m1 != 0) {
        break;
      }
      b0.base64_decode_block(dst);
      b1.template base64_decode_block<true>(dst + 48);
      src += 128;
      dst += 96;
    }
  }
  if (srclen >= 64) {
    const chartype *const srcend64 = srcinit + srclen - 64;
    while (src <= srcend64) {
      block64 b(src);
      uint64_t error = 0;
      const uint64_t badcharmask =
          b.to_base64_mask<base64_url, ignore_garbage, default_or_url>(&error);
      if (!ignore_garbage && error) {
        const size_t error_offset = trailing_zeroes(error);
        return {error_code::INVALID_BASE64_CHARACTER,
                size_t(src - srcinit + error_offset), size_t(dst - dstinit)};
      }
      src += 64;
      if (badcharmask != 0) {
        bufferptr += b.compress_block(badcharmask, bufferptr);
      } else if (bufferptr != buffer) {
        b.copy_block(bufferptr);
        bufferptr += 64;
      } else {
        b.template base64_decode_block<true>(dst);
        dst += 48;
      }
      if (bufferptr >= (block_size - 1) * 64 + buffer) {
        for (size_t i = 0; i < (block_size - 2); i++) {
          base64_decode_block(dst, buffer + i * 64);
          dst += 48;
        }
        base64_decode_block<true>(dst, buffer + (block_size - 2) * 64);
        dst += 48;
        std::memcpy(buffer, buffer + (block_size - 1) * 64,
                    64); // 64 might be too much
        bufferptr -= (block_size - 1) * 64;
      }
    }
  }

  char *buffer_start = buffer;
  // Optimization note: if this is almost full, then it is worth our
  // time, otherwise, we should just decode directly.
  int last_block = (int)((bufferptr - buffer_start) % 64);
  if (last_block != 0 && srcend - src + last_block >= 64) {

    while ((bufferptr - buffer_start) % 64 != 0 && src < srcend) {
      uint8_t val = to_base64[uint8_t(*src)];
      *bufferptr = char(val);
      if (!ignore_garbage &&
          (!scalar::base64::is_eight_byte(*src) || val > 64)) {
        return {error_code::INVALID_BASE64_CHARACTER, size_t(src - srcinit),
                size_t(dst - dstinit)};
      }
      bufferptr += (val <= 63);
      src++;
    }
  }

  for (; buffer_start + 64 <= bufferptr; buffer_start += 64) {
    // 8 payload bytes still in the buffer become 6 output bytes, which cover
    // the 4-byte tail of a wide store. Fewer than that, and the tail can
    // stick out past the real end.
    const size_t after = size_t(bufferptr - (buffer_start + 64));
    if (after >= 8) {
      base64_decode_block(dst, buffer_start);
    } else {
      base64_decode_block<true>(dst, buffer_start);
    }
    dst += 48;
  }
  if ((bufferptr - buffer_start) % 64 != 0) {
    while (buffer_start + 4 < bufferptr) {
      uint32_t triple = ((uint32_t(uint8_t(buffer_start[0])) << 3 * 6) +
                         (uint32_t(uint8_t(buffer_start[1])) << 2 * 6) +
                         (uint32_t(uint8_t(buffer_start[2])) << 1 * 6) +
                         (uint32_t(uint8_t(buffer_start[3])) << 0 * 6))
                        << 8;
#if !SIMDUTF_IS_BIG_ENDIAN
      triple = scalar::u32_swap_bytes(triple);
#endif
      std::memcpy(dst, &triple, 3);

      dst += 3;
      buffer_start += 4;
    }
    if (buffer_start + 4 <= bufferptr) {
      uint32_t triple = ((uint32_t(uint8_t(buffer_start[0])) << 3 * 6) +
                         (uint32_t(uint8_t(buffer_start[1])) << 2 * 6) +
                         (uint32_t(uint8_t(buffer_start[2])) << 1 * 6) +
                         (uint32_t(uint8_t(buffer_start[3])) << 0 * 6))
                        << 8;
#if !SIMDUTF_IS_BIG_ENDIAN
      triple = scalar::u32_swap_bytes(triple);
#endif
      std::memcpy(dst, &triple, 3);

      dst += 3;
      buffer_start += 4;
    }
    // we may have 1, 2 or 3 bytes left and we need to decode them so let us
    // backtrack
    int leftover = int(bufferptr - buffer_start);
    while (leftover > 0) {
      if (!ignore_garbage) {
        while (to_base64[uint8_t(*(src - 1))] == 64) {
          src--;
        }
      } else {
        while (to_base64[uint8_t(*(src - 1))] >= 64) {
          src--;
        }
      }
      src--;
      leftover--;
    }
  }
  if (src < srcend + equalsigns) {
    full_result r = scalar::base64::base64_tail_decode(
        dst, src, srcend - src, equalsigns, options, last_chunk_options);
    r = scalar::base64::patch_tail_result(
        r, size_t(src - srcinit), size_t(dst - dstinit), equallocation,
        full_input_length, last_chunk_options);
    // When is_partial(last_chunk_options) is true, we must either end with
    // the end of the stream (beyond whitespace) or right after a non-ignorable
    // character or at the very beginning of the stream.
    // See https://tc39.es/proposal-arraybuffer-base64/spec/#sec-frombase64
    if (is_partial(last_chunk_options) && r.error == error_code::SUCCESS &&
        r.input_count < full_input_length) {
      // First check if we can extend the input to the end of the stream
      while (r.input_count < full_input_length &&
             base64_ignorable(*(srcinit + r.input_count), options)) {
        r.input_count++;
      }
      // If we are still not at the end of the stream, then we must backtrack
      // to the last non-ignorable character.
      if (r.input_count < full_input_length) {
        while (r.input_count > 0 &&
               base64_ignorable(*(srcinit + r.input_count - 1), options)) {
          r.input_count--;
        }
      }
    }
    return r;
  }
  if (!ignore_garbage && equalsigns > 0) {
    if ((size_t(dst - dstinit) % 3 == 0) ||
        ((size_t(dst - dstinit) % 3) + 1 + equalsigns != 4)) {
      return {INVALID_BASE64_CHARACTER, equallocation, size_t(dst - dstinit),
              true};
    }
  }
  return {SUCCESS, srclen, size_t(dst - dstinit)};
}

} // namespace base64
} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf
