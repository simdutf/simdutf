#include "simdutf.h"

#include <array>
#include <cstring>
#include <memory>
#include <random>
#include <vector>

#include <tests/reference/encode_utf8.h>
#include <tests/reference/encode_utf16.h>
#include <tests/reference/validate_utf8.h>
#include <tests/reference/validate_utf16.h>
#include <tests/reference/validate_utf32.h>
#include <tests/helpers/random_int.h>
#include <tests/helpers/random_utf8.h>
#include <tests/helpers/random_utf16.h>
#include <tests/helpers/test.h>
#include <tests/helpers/utf16.h>

namespace {
std::array<size_t, 7> input_size{8, 16, 12, 64, 68, 128, 256};

template <typename Pointer> bool has_bom(Pointer *data, size_t size) {
  return (simdutf::BOM::check_bom(data, size) !=
          simdutf::encoding_type::unspecified);
}

template <typename RandomGenerator>
std::vector<uint8_t> generate_utf8(RandomGenerator random, size_t size) {
  // This is quite ugly workaround, but we expect almost none repeats
  while (true) {
    const auto generated = random.generate(size);
    if (!has_bom(generated.data(), generated.size())) {
      return generated;
    }
  }
}

template <typename RandomGenerator>
std::vector<char16_t> generate_utf16_le(RandomGenerator random, size_t size) {
  // This is quite ugly workaround, but we expect almost none repeats
  while (true) {
    const auto generated = random.generate_le(size);
    if (!has_bom(reinterpret_cast<const char *>(generated.data()),
                 generated.size())) {
      return generated;
    }
  }
}

template <typename RandomGenerator>
std::vector<uint16_t> generate_u16(RandomGenerator &random, size_t count) {
  std::vector<uint16_t> result;
  result.reserve(count);

  union {
    uint16_t word[2];
    char string[4];
  } first;

  static_assert(sizeof(first) == 4, "union got an unexpected size");

  // Make sure our random data does not start with a BOM marker.
  do {
    first.word[0] = random();
    first.word[1] = random();
  } while (has_bom(first.string, 4));

  result.push_back(first.word[0]);
  result.push_back(first.word[1]);
  for (size_t i = 2; i < count; i++) {
    result.push_back(random());
  }

  return result;
}

template <typename RandomGenerator>
std::vector<uint32_t> generate_u32(RandomGenerator &random, size_t count) {
  std::vector<uint32_t> result;
  result.reserve(count);

  union {
    uint32_t word;
    char string[4];
  } first;

  static_assert(sizeof(first) == 4, "union got an unexpected size");

  // Make sure our random data does not start with a BOM marker.
  do {
    first.word = random();
  } while (has_bom(first.string, 4));

  result.push_back(first.word);
  for (size_t i = 1; i < count; i++) {
    result.push_back(random());
  }

  return result;
}

} // namespace

TEST(issue_519) {
  // implementation icelake gave 0
  // implementation haswell gave 0
  // implementation westmere gave 0
  // implementation fallback gave 1
  std::vector<unsigned char> data{
      32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,  32,  32,
      32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 223, 164, 32,
      32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,  32,  32,
      32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,  32,  32,
  };
  const auto r =
      implementation.detect_encodings((const char *)data.data(), data.size());
  ASSERT_EQUAL(r, 1);
}

TEST(issue_516) {
  std::vector<unsigned char> data{0x20, 0xd8, 0x00, 0x00};
  const auto r =
      implementation.detect_encodings((const char *)data.data(), data.size());
  ASSERT_EQUAL(r, 0);
}

TEST(issue818) {
  std::string data = "\xEF\xBB\xBF";
  const auto r = simdutf::BOM::check_bom(data.data(), data.size());
  ASSERT_EQUAL(r, simdutf::encoding_type::UTF8);
}

TEST(boommmmm) {
  const char *utf8_bom = "\xef\xbb\xbf";
  const char *utf16be_bom = "\xfe\xff";
  const char *utf16le_bom = "\xff\xfe";
  ASSERT_EQUAL(implementation.detect_encodings(utf8_bom, 3),
               simdutf::encoding_type::UTF8);
  ASSERT_EQUAL(implementation.detect_encodings(utf16be_bom, 2),
               simdutf::encoding_type::UTF16_BE);
  ASSERT_EQUAL(implementation.detect_encodings(utf16le_bom, 2),
               simdutf::encoding_type::UTF16_LE);
}

#if !SIMDUTF_IS_BIG_ENDIAN
TEST(issue_627) {
  std::vector<unsigned char> data{
      251, 219, 37,  222, 0,   199, 218, 0,   157, 0,   0, 255, 8,   8,
      8,   8,   227, 8,   8,   8,   8,   8,   8,   255, 0, 0,   248, 0,
      255, 8,   8,   8,   8,   10,  8,   8,   8,   8,   8, 8,   8,   0,
      219, 0,   0,   122, 0,   255, 0,   5,   0,   0,   0, 0,   0,   0,
      0,   255, 243, 159, 129, 172, 1,   219, 37,  222,
  };
  const auto r1 =
      implementation.detect_encodings((const char *)data.data(), data.size());
  ASSERT_EQUAL(r1, 2);

  std::vector<unsigned char> data2{
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 219,
  };
  const auto r2 =
      implementation.detect_encodings((const char *)data2.data(), data2.size());
  ASSERT_EQUAL(r2, 0);
}
#endif

TEST_LOOP(pure_utf8_ASCII) {
  simdutf::tests::helpers::random_utf8 random(seed, 1, 0, 0, 0);

  for (size_t size : input_size) {
    const auto generated = generate_utf8(random, size);
    auto expected =
        simdutf::encoding_type::UTF8 | simdutf::encoding_type::UTF16_LE;
    auto actual = implementation.detect_encodings(
        reinterpret_cast<const char *>(generated.data()), size);
    ASSERT_EQUAL(actual, expected);
  }
}

TEST_LOOP(pure_utf16_ASCII) {
  simdutf::tests::helpers::RandomInt random(0, 127, seed);

  for (size_t size : input_size) {
    const auto generated = generate_u16(random, size / 2);
    auto expected =
        simdutf::encoding_type::UTF8 | simdutf::encoding_type::UTF16_LE;
    auto actual = implementation.detect_encodings(
        reinterpret_cast<const char *>(generated.data()), size);
    ASSERT_TRUE((actual & expected) ==
                expected); // Must be at least UTF8 and UTF16_LE.
  }
}

TEST_LOOP(pure_utf32_ASCII) {
  simdutf::tests::helpers::RandomInt random(0, 0x7f, seed);

  for (size_t size : input_size) {
    const auto generated = generate_u32(random, size / 4);
    auto expected = simdutf::encoding_type::UTF8 |
                    simdutf::encoding_type::UTF16_LE |
                    simdutf::encoding_type::UTF32_LE;
    auto actual = implementation.detect_encodings(
        reinterpret_cast<const char *>(generated.data()), size);
    ASSERT_TRUE((actual & expected) ==
                expected); // Must be at least UTF8 and UTF16_LE and UTF32_LE.
  }
}

#if 0 // XXX
TEST_LOOP(no_utf8_bytes_no_surrogates) {
  simdutf::tests::helpers::RandomIntRanges random(
      {{0x007f, 0xd800 - 1}, {0xe000, 0xffff}}, seed);

  for (size_t size : input_size) {
    auto generated = generate_u32(random, size / 4);
    if (!simdutf::match_system(simdutf::endianness::LITTLE)) {
        for (auto& val: generated) {
            val = ((val & 0xff) << 8) | (val >> 8);
        }
    }

    auto expected =
        simdutf::encoding_type::UTF16_LE | simdutf::encoding_type::UTF32_LE;
    auto actual = implementation.detect_encodings(
        reinterpret_cast<const char *>(generated.data()), size);
    ASSERT_EQUAL((actual & expected),
                 expected); // Must be at least UTF16_LE and UTF32_LE.
  }
}
#endif

TEST_LOOP(two_utf8_bytes) {
  simdutf::tests::helpers::random_utf8 random(seed, 0, 1, 0, 0);

  for (size_t size : input_size) {
    const auto generated = generate_utf8(random, size);
    auto expected =
        simdutf::encoding_type::UTF8 | simdutf::encoding_type::UTF16_LE;
    auto actual = implementation.detect_encodings(
        reinterpret_cast<const char *>(generated.data()), size);
    if (actual != expected) {
      if ((actual & simdutf::encoding_type::UTF8) == 0) {
        puts("failed to detect valid UTF-8.");
      }
      if ((actual & simdutf::encoding_type::UTF16_LE) == 0) {
        puts("failed to detect valid UTF-16LE.");
      }
    }
    ASSERT_TRUE((actual & expected) ==
                expected); // Must be at least UTF8 and UTF16_LE.
  }
}

TEST_LOOP(utf_16_surrogates) {
  simdutf::tests::helpers::random_utf16 random(seed, 0, 1);

  for (size_t size : input_size) {
    const auto generated = generate_utf16_le(random, size / 2);
    auto expected = simdutf::encoding_type::UTF16_LE;
    auto actual = implementation.detect_encodings(
        reinterpret_cast<const char *>(generated.data()), size);
    ASSERT_TRUE((actual & expected) == expected); // Must be at least UTF16_LE.
  }
}

TEST_LOOP(utf32_surrogates) {
  simdutf::tests::helpers::RandomInt random_prefix(0x10000, 0x10ffff, seed);
  simdutf::tests::helpers::RandomInt random_suffix(0xd800, 0xdfff, seed);

  for (size_t size : input_size) {
    std::vector<uint32_t> generated;
    for (unsigned int i = 0; i < size / 4; i++) {
      generated.push_back((random_prefix() & 0xffff0000) + random_suffix());
    }
    auto expected = simdutf::encoding_type::UTF32_LE;
    auto actual = implementation.detect_encodings(
        reinterpret_cast<const char *>(generated.data()), size);
    ASSERT_TRUE((actual & expected) == expected); // Must be at least UTF32_LE.
  }
}

TEST_LOOP(edge_surrogate) {
  simdutf::tests::helpers::RandomInt random(0x10000, 0x10ffff, seed);

  const size_t size = 512;
  std::vector<uint16_t> generated(size / 2, 0);
  unsigned int i = 31;
  while (i + 32 < (size / 2) - 1) {
    char16_t W1;
    char16_t W2;
    ASSERT_EQUAL(simdutf::tests::reference::utf16::encode(random(), W1, W2), 2);
    generated[i + 0] = to_utf16le(W1);
    generated[i + 1] = to_utf16le(W2);
    i += 32;
  }
  auto expected = simdutf::encoding_type::UTF16_LE;
  auto actual = implementation.detect_encodings(
      reinterpret_cast<const char *>(generated.data()), size);
  ASSERT_TRUE((actual & expected) == expected); // Must be at least UTF16_LE.
}

TEST_LOOP(tail_utf8) {
  simdutf::tests::helpers::random_utf8 random(seed, 0, 0, 1, 0);
  std::array<size_t, 5> multiples_three{12, 54, 66, 126, 252};
  for (size_t size : multiples_three) {
    const auto generated = generate_utf8(random, size);
    auto expected =
        simdutf::encoding_type::UTF8 | simdutf::encoding_type::UTF16_LE;
    auto actual = implementation.detect_encodings(
        reinterpret_cast<const char *>(generated.data()), size);
    ASSERT_TRUE((actual & expected) ==
                expected); // Must be at least UTF8 and UTF16_LE.
  }
}

#if !SIMDUTF_IS_BIG_ENDIAN
namespace {
// The result that detect_encodings must give, computed with the reference
// validators.
int reference_detect(const std::vector<uint8_t> &data) {
  const char *p = reinterpret_cast<const char *>(data.data());
  const size_t len = data.size();
  const auto bom = simdutf::BOM::check_bom(p, len);
  if (bom != simdutf::encoding_type::unspecified) {
    return bom;
  }
  int out = 0;
  if (simdutf::tests::reference::validate_utf8(p, len)) {
    out |= simdutf::encoding_type::UTF8;
  }
  if (len % 2 == 0) {
    std::vector<char16_t> u16(len / 2);
    if (len != 0) {
      std::memcpy(u16.data(), p, len);
    }
    if (simdutf::tests::reference::validate_utf16(simdutf::endianness::LITTLE,
                                                  u16.data(), u16.size())) {
      out |= simdutf::encoding_type::UTF16_LE;
    }
  }
  if (len % 4 == 0) {
    std::vector<char32_t> u32(len / 4);
    if (len != 0) {
      std::memcpy(u32.data(), p, len);
    }
    if (simdutf::tests::reference::validate_utf32(u32.data(), u32.size())) {
      out |= simdutf::encoding_type::UTF32_LE;
    }
  }
  return out;
}

void check_against_reference(const simdutf::implementation &implementation,
                             const std::vector<uint8_t> &data) {
  // An exact-size copy so that the sanitizers catch reads past the end. An
  // empty input gets a valid pointer to a byte outside of it.
  std::unique_ptr<char[]> copy(new char[data.empty() ? 1 : data.size()]);
  if (!data.empty()) {
    std::memcpy(copy.get(), data.data(), data.size());
  }
  const int expected = reference_detect(data);
  const int actual = implementation.detect_encodings(copy.get(), data.size());
  if (actual != expected) {
    printf("detect_encodings gave %d, expected %d, input (%zu bytes):\n",
           actual, expected, data.size());
    for (uint8_t b : data) {
      printf("%02x ", b);
    }
    printf("\n");
  }
  ASSERT_EQUAL(actual, expected);
}

void put_u16le(std::vector<uint8_t> &data, size_t unit, uint16_t value) {
  data[2 * unit] = uint8_t(value);
  data[2 * unit + 1] = uint8_t(value >> 8);
}

void put_u32le(std::vector<uint8_t> &data, size_t index, uint32_t value) {
  for (size_t i = 0; i < 4; i++) {
    data[4 * index + i] = uint8_t(value >> (8 * i));
  }
}

// Lengths around the 64-byte blocks and the 1024-byte stride at which some
// implementations stop checking the encodings that already failed.
const std::vector<size_t> edge_lengths{
    0,    1,    2,    3,    4,    6,    14,   15,   16,   17,  30,   31,   32,
    33,   60,   62,   63,   64,   65,   66,   68,   78,   79,  80,   81,   96,
    124,  126,  127,  128,  129,  130,  132,  191,  192,  196, 1020, 1024, 1026,
    1028, 1088, 1090, 1092, 2048, 2050, 2116, 4096, 4097, 4100};

std::vector<size_t> positions_near_edges(size_t count) {
  std::vector<size_t> out;
  for (size_t p = 0; p < count; p++) {
    const size_t m = p % 32;
    if (p < 3 || p + 3 >= count || m <= 1 || m >= 30) {
      out.push_back(p);
    }
  }
  return out;
}

// Writes 'character' again and again from 'from' on, as long as it fits.
void fill_utf8(std::vector<uint8_t> &data, size_t from,
               const std::vector<uint8_t> &character) {
  for (size_t i = from; i + character.size() <= data.size();
       i += character.size()) {
    std::copy(character.begin(), character.end(), data.begin() + i);
  }
}

// Returns 'len' bytes of valid UTF-8, mostly ASCII, that are known not to be
// UTF-16LE or UTF-32LE once the first 'stride' bytes are read (stride is 1024
// or 2048). An odd length rules out both from the start. For an even length,
// U+0628 (0xd8 0xa8) at an odd index puts a high surrogate without a low
// surrogate in UTF-16LE: at byte 1 for a stride of 1024, at byte 1501 for a
// stride of 2048 (ASCII alone is valid UTF-16LE). The UTF-32LE values are too
// large.
std::vector<uint8_t> utf8_only_after(size_t len, size_t stride) {
  std::vector<uint8_t> data(len, 'x');
  if (len % 2 == 0) {
    const size_t at = stride == 1024 ? 1 : 1501;
    data[at] = 0xd8;
    data[at + 1] = 0xa8;
  }
  return data;
}

// Lengths to use with utf8_only_after(len, stride).
std::vector<size_t> utf8_only_lengths(size_t stride, bool all_lengths) {
  if (stride == 2048) {
    return {2114, 2116, 2118, 2120, 2178, 2180, 3000, 4096};
  }
  if (!all_lengths) {
    return {1089, 1090, 1091, 1092, 1095, 1096, 1151, 1152, 2048, 2049, 2050};
  }
  std::vector<size_t> out;
  for (size_t len = 1025; len <= 2100; len++) {
    out.push_back(len);
  }
  return out;
}
} // namespace

TEST(detect_encodings_empty) {
  const char byte = 'x';
  ASSERT_EQUAL(implementation.detect_encodings(&byte, 0),
               simdutf::encoding_type::UTF8 | simdutf::encoding_type::UTF16_LE |
                   simdutf::encoding_type::UTF32_LE);
}

TEST(detect_encodings_all_ascii_and_odd_lengths) {
  for (size_t len = 0; len <= 300; len++) {
    std::vector<uint8_t> data(len);
    for (size_t i = 0; i < len; i++) {
      data[i] = uint8_t('a' + i % 26);
    }
    check_against_reference(implementation, data);
    // ASCII that is also UTF-32LE (and UTF-16LE)
    std::fill(data.begin(), data.end(), 0);
    for (size_t i = 0; i < len; i += 4) {
      data[i] = uint8_t('a' + i % 26);
    }
    check_against_reference(implementation, data);
  }
  for (size_t len : edge_lengths) {
    // a single non-ASCII byte at the end
    std::vector<uint8_t> data(len, 'x');
    if (len > 0) {
      data[len - 1] = 0x80;
    }
    check_against_reference(implementation, data);
  }
}

TEST(detect_encodings_utf16_surrogates_at_block_edges) {
  for (size_t len : edge_lengths) {
    const size_t units = len / 2;
    for (size_t u : positions_near_edges(units)) {
      for (int pattern = 0; pattern < 5; pattern++) {
        std::vector<uint8_t> data(len, 0);
        if (len % 2 == 1) {
          data[len - 1] = 'z';
        }
        switch (pattern) {
        case 0: // valid pair
          if (u + 1 >= units) {
            continue;
          }
          put_u16le(data, u, 0xd800 + uint16_t(u % 0x400));
          put_u16le(data, u + 1, 0xdc00 + uint16_t(u % 0x400));
          break;
        case 1: // lone high surrogate
          put_u16le(data, u, 0xdbff);
          break;
        case 2: // lone low surrogate
          put_u16le(data, u, 0xdc00);
          break;
        case 3: // low then high
          if (u + 1 >= units) {
            continue;
          }
          put_u16le(data, u, 0xdfff);
          put_u16le(data, u + 1, 0xd800);
          break;
        default: // two high surrogates then a low one
          if (u + 2 >= units) {
            continue;
          }
          put_u16le(data, u, 0xd800);
          put_u16le(data, u + 1, 0xd801);
          put_u16le(data, u + 2, 0xdc01);
          break;
        }
        check_against_reference(implementation, data);
        // the same with ASCII instead of zeros around it
        for (size_t i = 0; i < len; i++) {
          if (data[i] == 0) {
            data[i] = ' ';
          }
        }
        check_against_reference(implementation, data);
      }
    }
  }
}

TEST(detect_encodings_utf32_values_at_block_edges) {
  const uint32_t values[] = {
      0x7f,     0x80,       0xd7ff,     0xd800,     0xdbff,     0xdc00,
      0xdfff,   0xe000,     0xffff,     0x10000,    0x1d800,    0x10ffff,
      0x110000, 0x7fffffff, 0x80000000, 0xffffffff, 0xffff2000, 0xfffff7ff};
  for (size_t len : edge_lengths) {
    const size_t count = len / 4;
    for (size_t i : positions_near_edges(count)) {
      for (uint32_t v : values) {
        std::vector<uint8_t> data(len, 0);
        put_u32le(data, i, v);
        check_against_reference(implementation, data);
      }
    }
  }
}

TEST(detect_encodings_utf8_at_block_edges) {
  const std::vector<std::vector<uint8_t>> sequences{
      {0xc3, 0xa9},             // U+00E9
      {0xe2, 0x82, 0xac},       // U+20AC
      {0xf0, 0x9f, 0x98, 0x80}, // U+1F600
      {0xf4, 0x8f, 0xbf, 0xbf}, // U+10FFFF
      {0xe2, 0x82},             // truncated
      {0x80},                   // lone continuation
      {0xc0, 0x80},             // overlong
      {0xed, 0xa0, 0x80},       // surrogate
      {0xf4, 0x90, 0x80, 0x80}, // too large
      {0xff}};
  for (size_t len : edge_lengths) {
    for (size_t p = 0; p < len; p++) {
      const size_t m = p % 64;
      if (!(p < 4 || p + 4 >= len || m <= 2 || m >= 60)) {
        continue;
      }
      for (const auto &seq : sequences) {
        if (p + seq.size() > len) {
          continue;
        }
        std::vector<uint8_t> data(len, 'x');
        std::copy(seq.begin(), seq.end(), data.begin() + p);
        check_against_reference(implementation, data);
      }
    }
  }
}

TEST(detect_encodings_boms) {
  const std::vector<std::vector<uint8_t>> prefixes{{0xef, 0xbb, 0xbf},
                                                   {0xff, 0xfe},
                                                   {0xfe, 0xff},
                                                   {0xff, 0xfe, 0x00, 0x00},
                                                   {0xff, 0xfe, 0x00, 0x01},
                                                   {0xef, 0xbb}};
  for (const auto &prefix : prefixes) {
    for (size_t len : edge_lengths) {
      for (uint8_t fill : {uint8_t(0), uint8_t('a'), uint8_t(0xff)}) {
        std::vector<uint8_t> data(prefix);
        data.resize(prefix.size() + len, fill);
        check_against_reference(implementation, data);
      }
    }
  }
}

TEST(detect_encodings_late_errors) {
  // An encoding fails late in a long input, after another one has failed.
  for (size_t len : {size_t(1100), size_t(2200), size_t(4096), size_t(5000)}) {
    for (size_t pos :
         {size_t(0), size_t(63), size_t(64), size_t(1023), size_t(1024),
          size_t(1088), size_t(2047), len - 64, len - 2}) {
      if (pos + 1 >= len) {
        continue;
      }
      for (int pattern = 0; pattern < 3; pattern++) {
        std::vector<uint8_t> data(len, 0);
        for (size_t i = 0; i < len; i += 2) {
          data[i] = 'a';
        }
        // UTF-32 fails at once (0x00610061), UTF-8 and UTF-16 stay valid.
        if (pattern == 0) {
          data[pos] = 0xff; // UTF-8 fails at pos
        } else if (pattern == 1) {
          put_u16le(data, pos / 2, 0xd800); // UTF-8 and UTF-16 fail at pos
        } else {
          data[pos] = 0xc3; // UTF-8 still valid, UTF-16 too
          data[pos + 1] = 0xa9;
        }
        check_against_reference(implementation, data);
      }
    }
  }
}

TEST_LOOP(detect_encodings_random_against_reference) {
  std::mt19937 gen(seed);
  auto rnd = [&gen](uint32_t n) { return uint32_t(gen() % n); };
  for (int round = 0; round < 20; round++) {
    const size_t len = rnd(4) == 0 ? 1000 + rnd(1200) : rnd(300);
    std::vector<uint8_t> data;
    const uint32_t kind = rnd(4);
    while (data.size() < len + 4) {
      uint32_t cp = rnd(4) == 0 ? rnd(0x110000) : rnd(0x800);
      if (cp >= 0xd800 && cp <= 0xdfff) {
        cp = 0xfffd;
      }
      if (kind == 0) { // UTF-8
        simdutf::tests::reference::utf8::encode(
            cp, [&data](uint8_t byte) { data.push_back(byte); });
      } else if (kind == 1) { // UTF-16LE
        char16_t w1, w2;
        const int n = simdutf::tests::reference::utf16::encode(cp, w1, w2);
        data.push_back(uint8_t(w1));
        data.push_back(uint8_t(w1 >> 8));
        if (n == 2) {
          data.push_back(uint8_t(w2));
          data.push_back(uint8_t(w2 >> 8));
        }
      } else if (kind == 2) { // UTF-32LE
        for (size_t i = 0; i < 4; i++) {
          data.push_back(uint8_t(cp >> (8 * i)));
        }
      } else {
        data.push_back(uint8_t(rnd(256)));
      }
    }
    data.resize(len);
    const uint32_t mutations = rnd(3);
    for (uint32_t m = 0; m < mutations && len > 0; m++) {
      const uint8_t replacements[] = {0x00, 0x80, 0xd8, 0xdb,
                                      0xdc, 0xdf, 0xf4, 0xff};
      data[rnd(uint32_t(len))] = replacements[rnd(8)];
    }
    check_against_reference(implementation, data);
  }
}

// Once UTF-8 is the only encoding left, some implementations pass the rest of
// the input to validate_utf8, from a multiple of 1024 bytes or from the start
// of the character that straddles it.
TEST(detect_encodings_utf8_character_across_stride) {
  const std::vector<std::vector<uint8_t>> characters{
      {0xc3, 0xa9}, {0xe2, 0x82, 0xac}, {0xf0, 0x9f, 0x98, 0x80}};
  for (size_t stride : {size_t(1024), size_t(2048)}) {
    for (size_t len : utf8_only_lengths(stride, true)) {
      for (const auto &character : characters) {
        // Every shift puts a different byte of a character at 'stride'.
        for (size_t shift = 0; shift < character.size(); shift++) {
          std::vector<uint8_t> data = utf8_only_after(len, stride);
          fill_utf8(data, stride - 64 + shift, character);
          check_against_reference(implementation, data);
        }
      }
    }
  }
}

TEST(detect_encodings_utf8_errors_across_stride) {
  const std::vector<std::vector<uint8_t>> sequences{
      {0xc3},                         // truncated
      {0xe2, 0x82},                   // truncated
      {0xf0, 0x9f, 0x98},             // truncated
      {0x80},                         // lone continuation
      {0xbf},                         // lone continuation
      {0xc0, 0x80},                   // overlong
      {0xc1, 0xbf},                   // overlong
      {0xe0, 0x9f, 0xbf},             // overlong
      {0xf0, 0x8f, 0xbf, 0xbf},       // overlong
      {0xed, 0xa0, 0x80},             // surrogate
      {0xed, 0xbf, 0xbf},             // surrogate
      {0xf4, 0x90, 0x80, 0x80},       // too large
      {0xf5, 0x80, 0x80, 0x80},       // too large
      {0xf0, 0x9f, 0x98, 0x80, 0x80}, // four continuation bytes
      {0xff},
      {0xc3, 0xa9},              // U+00E9
      {0xe2, 0x82, 0xac},        // U+20AC
      {0xf0, 0x9f, 0x98, 0x80},  // U+1F600
      {0xf4, 0x8f, 0xbf, 0xbf}}; // U+10FFFF
  const std::vector<std::vector<uint8_t>> backgrounds{
      {'x'}, {0xc3, 0xa9}, {0xe2, 0x82, 0xac}};
  for (size_t stride : {size_t(1024), size_t(2048)}) {
    for (size_t len : utf8_only_lengths(stride, false)) {
      for (const auto &background : backgrounds) {
        for (const auto &seq : sequences) {
          // The sequence ends before 'stride', straddles it or follows it.
          for (size_t p = stride - 5; p <= stride + 2; p++) {
            std::vector<uint8_t> data = utf8_only_after(len, stride);
            fill_utf8(data, stride - 64, background);
            std::copy(seq.begin(), seq.end(), data.begin() + p);
            check_against_reference(implementation, data);
          }
        }
      }
    }
  }
}
#endif // !SIMDUTF_IS_BIG_ENDIAN

TEST_MAIN
