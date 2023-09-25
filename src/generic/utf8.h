#include "scalar/utf8.h"

namespace simdutf {
namespace SIMDUTF_IMPLEMENTATION {
namespace {
namespace utf8 {

using namespace simd;

//gcc 11.3.1 Before
/* count_utf8+haswell, input size: 446908, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utf8.txt
   0.220 ins/byte,    0.070 cycle/byte,   45.771 GB/s (36.7 %),     3.210 GHz,    3.131 ins/cycle 
   0.226 ins/char,    0.072 cycle/char,   44.538 Gc/s (36.7 %)     1.03 byte/char 
WARNING: Measurements are noisy, try increasing iteration count (-I).
count_utf8+icelake, input size: 446908, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utf8.txt
   0.083 ins/byte,    0.048 cycle/byte,   65.376 GB/s (11.8 %),     3.116 GHz,    1.746 ins/cycle 
   0.086 ins/char,    0.049 cycle/char,   63.614 Gc/s (11.8 %)     1.03 byte/char 
WARNING: Measurements are noisy, try increasing iteration count (-I).
count_utf8+westmere, input size: 446908, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utf8.txt
   0.470 ins/byte,    0.107 cycle/byte,   30.071 GB/s (0.9 %),     3.203 GHz,    4.409 ins/cycle 
   0.483 ins/char,    0.109 cycle/char,   29.260 Gc/s (0.9 %)     1.03 byte/char  */

// after
/* count_utf8+haswell, input size: 446908, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utf8.txt
   0.220 ins/byte,    0.071 cycle/byte,   45.261 GB/s (13.4 %),     3.209 GHz,    3.098 ins/cycle 
   0.226 ins/char,    0.073 cycle/char,   44.042 Gc/s (13.4 %)     1.03 byte/char 
WARNING: Measurements are noisy, try increasing iteration count (-I).
count_utf8+icelake, input size: 446908, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utf8.txt
   0.083 ins/byte,    0.048 cycle/byte,   65.414 GB/s (13.0 %),     3.117 GHz,    1.747 ins/cycle 
   0.086 ins/char,    0.049 cycle/char,   63.651 Gc/s (13.0 %)     1.03 byte/char 
WARNING: Measurements are noisy, try increasing iteration count (-I).
count_utf8+westmere, input size: 446908, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utf8.txt
   0.438 ins/byte,    0.108 cycle/byte,   29.573 GB/s (0.5 %),     3.203 GHz,    4.047 ins/cycle 
   0.451 ins/char,    0.111 cycle/char,   28.776 Gc/s (0.5 %)     1.03 byte/char  */

simdutf_really_inline size_t count_code_points(const char* in, size_t size) {
    size_t pos = 0;
    size_t count = 0;
    for(;pos + 64 <= size; pos += 64) {
      simd8x64<int8_t> input(reinterpret_cast<const int8_t *>(in + pos));
/*       uint64_t utf8_continuation_mask = input.lt(-65 + 1);
      count += 64 - count_ones(utf8_continuation_mask);
 */   uint64_t utf8_continuation_mask = input.gt(-65);
      count += count_ones(utf8_continuation_mask);
    }
    return count + scalar::utf8::count_code_points(in + pos, size - pos);
}

// Clang 15.0.7 before

/* count_utf8+fallback, input size: 446908, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utf8.txt
   4.376 ins/byte,    1.321 cycle/byte,    2.418 GB/s (0.3 %),     3.194 GHz,    3.313 ins/cycle 
   4.497 ins/char,    1.357 cycle/char,    2.353 Gc/s (0.3 %)     1.03 byte/char 
count_utf8+haswell, input size: 446908, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utf8.txt
   0.220 ins/byte,    0.080 cycle/byte,   40.222 GB/s (2.3 %),     3.210 GHz,    2.752 ins/cycle 
   0.226 ins/char,    0.082 cycle/char,   39.138 Gc/s (2.3 %)     1.03 byte/char 
count_utf8+icelake, input size: 446908, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utf8.txt
   0.075 ins/byte,    0.048 cycle/byte,   65.433 GB/s (12.2 %),     3.122 GHz,    1.580 ins/cycle 
   0.077 ins/char,    0.049 cycle/char,   63.670 Gc/s (12.2 %)     1.03 byte/char 
WARNING: Measurements are noisy, try increasing iteration count (-I).
count_utf8+westmere, input size: 446908, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utf8.txt
   0.470 ins/byte,    0.103 cycle/byte,   31.178 GB/s (7.5 %),     3.207 GHz,    4.567 ins/cycle 
   0.483 ins/char,    0.106 cycle/char,   30.338 Gc/s (7.5 %)     1.03 byte/char  */

//after
/* count_utf8+fallback, input size: 446908, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utf8.txt
   4.376 ins/byte,    1.321 cycle/byte,    2.418 GB/s (0.3 %),     3.194 GHz,    3.313 ins/cycle 
   4.497 ins/char,    1.357 cycle/char,    2.353 Gc/s (0.3 %)     1.03 byte/char 
count_utf8+haswell, input size: 446908, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utf8.txt
   0.235 ins/byte,    0.074 cycle/byte,   43.271 GB/s (12.4 %),     3.212 GHz,    3.169 ins/cycle 
   0.242 ins/char,    0.076 cycle/char,   42.106 Gc/s (12.4 %)     1.03 byte/char 
WARNING: Measurements are noisy, try increasing iteration count (-I).
count_utf8+icelake, input size: 446908, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utf8.txt
   0.075 ins/byte,    0.048 cycle/byte,   65.404 GB/s (12.3 %),     3.122 GHz,    1.579 ins/cycle 
   0.077 ins/char,    0.049 cycle/char,   63.642 Gc/s (12.3 %)     1.03 byte/char 
WARNING: Measurements are noisy, try increasing iteration count (-I).
count_utf8+westmere, input size: 446908, iterations: 30000, dataset: /home/leorio/unicode_lipsum/wikipedia_mars/french.utf8.txt
   0.392 ins/byte,    0.097 cycle/byte,   33.055 GB/s (4.9 %),     3.207 GHz,    4.037 ins/cycle 
   0.402 ins/char,    0.100 cycle/char,   32.165 Gc/s (4.9 %)     1.03 byte/char  */


simdutf_really_inline size_t utf16_length_from_utf8(const char* in, size_t size) {
    size_t pos = 0;
    size_t count = 0;
    // This algorithm could no doubt be improved!
    for(;pos + 64 <= size; pos += 64) {
      simd8x64<int8_t> input(reinterpret_cast<const int8_t *>(in + pos));
      uint64_t utf8_continuation_mask = input.lt(-65 + 1);
      // We count one word for anything that is not a continuation (so
      // leading bytes).
      count += 64 - count_ones(utf8_continuation_mask);
      int64_t utf8_4byte = input.gteq_unsigned(240);
      count += count_ones(utf8_4byte);
    }
    return count + scalar::utf8::utf16_length_from_utf8(in + pos, size - pos);
}


simdutf_really_inline size_t utf32_length_from_utf8(const char* in, size_t size) {
    return count_code_points(in, size);
}
} // utf8 namespace
} // unnamed namespace
} // namespace SIMDUTF_IMPLEMENTATION
} // namespace simdutf
