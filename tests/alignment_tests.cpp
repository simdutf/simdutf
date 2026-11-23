#include "simdutf.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <random>
#include <vector>

#include <tests/helpers/test.h>

namespace {

constexpr std::array<size_t, 4> sizes_in_bytes{65535, 65536, 65603, 70001};
constexpr size_t every_offset_size = 65603;

uint32_t random_code_point(std::mt19937 &gen) {
  std::uniform_int_distribution<int> kind(0, 9);
  switch (kind(gen)) {
  case 0:
    return std::uniform_int_distribution<uint32_t>(0x80, 0x7FF)(gen);
  case 1:
    return std::uniform_int_distribution<uint32_t>(0x800, 0xD7FF)(gen);
  case 2:
    return std::uniform_int_distribution<uint32_t>(0x10000, 0x10FFFF)(gen);
  default:
    return std::uniform_int_distribution<uint32_t>(0, 0x7F)(gen);
  }
}

std::vector<char> make_utf8(std::mt19937 &gen, size_t n) {
  std::vector<char> out;
  while (out.size() < n) {
    const uint32_t c = random_code_point(gen);
    char buf[4];
    const size_t k = simdutf::convert_utf32_to_utf8(
        reinterpret_cast<const char32_t *>(&c), 1, buf);
    if (out.size() + k > n) {
      out.push_back('a');
      continue;
    }
    out.insert(out.end(), buf, buf + k);
  }
  return out;
}

std::vector<char16_t> make_utf16(std::mt19937 &gen, size_t n, bool big_endian) {
  std::vector<char16_t> out;
  while (out.size() < n) {
    const uint32_t c = random_code_point(gen);
    char16_t buf[2];
    const size_t k = simdutf::convert_utf32_to_utf16le(
        reinterpret_cast<const char32_t *>(&c), 1, buf);
    if (out.size() + k > n) {
      out.push_back(u'a');
      continue;
    }
    out.insert(out.end(), buf, buf + k);
  }
  if (big_endian) {
    simdutf::change_endianness_utf16(out.data(), out.size(), out.data());
  }
  return out;
}

std::vector<char32_t> make_utf32(std::mt19937 &gen, size_t n) {
  std::vector<char32_t> out(n);
  for (auto &c : out) {
    c = random_code_point(gen);
  }
  return out;
}

template <typename T> class aligned_buffer {
  std::vector<T> storage;
  T *base;

public:
  explicit aligned_buffer(size_t n) : storage(n + 192 / sizeof(T)) {
    const uintptr_t addr = reinterpret_cast<uintptr_t>(storage.data());
    base = storage.data() + ((64 - addr % 64) % 64) / sizeof(T);
  }
  T *at(size_t offset_in_bytes, const std::vector<T> &content) {
    T *p = base + offset_in_bytes / sizeof(T);
    std::memcpy(p, content.data(), content.size() * sizeof(T));
    return p;
  }
};

template <typename T>
std::vector<size_t> positions(size_t size_in_bytes, size_t offset_in_bytes,
                              size_t len) {
  if (size_in_bytes != every_offset_size && offset_in_bytes > 1 &&
      offset_in_bytes < 60) {
    return {};
  }
  const size_t split = ((64 - offset_in_bytes % 64) % 64) / sizeof(T);
  std::vector<size_t> out;
  if (size_in_bytes != every_offset_size) {
    out = {0, 1, len - 2, len - 1};
  }
  for (size_t i = (split >= 4 ? split - 4 : 0); i <= split + 4; i++) {
    out.push_back(i);
  }
  return out;
}

bool same(const simdutf::result &a, const simdutf::result &b) {
  return a.error == b.error && a.count == b.count;
}

bool same(const simdutf::utf8_result &a, const simdutf::utf8_result &b) {
  return a.error == b.error && a.input_count == b.input_count &&
         a.continuation_count == b.continuation_count &&
         a.four_byte_count == b.four_byte_count;
}

} // namespace

TEST(utf8_and_latin1_functions_at_every_offset) {
  std::mt19937 gen(1234);
  const uint8_t bad[] = {0x80, 0xC2, 0xED, 0xF4, 0xFF};
  for (size_t n : sizes_in_bytes) {
    const std::vector<char> valid = make_utf8(gen, n);
    std::vector<char> ascii(n);
    for (auto &c : ascii) {
      c = char(std::uniform_int_distribution<int>(0, 0x7F)(gen));
    }
    aligned_buffer<char> buf(n);
    for (size_t off = 0; off < 64; off++) {
      const char *p = buf.at(off, valid);
      ASSERT_EQUAL(implementation.count_utf8(p, n),
                   simdutf::scalar::utf8::count_code_points(p, n));
      ASSERT_EQUAL(implementation.utf16_length_from_utf8(p, n),
                   simdutf::scalar::utf8::utf16_length_from_utf8(p, n));
      ASSERT_EQUAL(implementation.utf32_length_from_utf8(p, n),
                   simdutf::scalar::utf8::count_code_points(p, n));
      ASSERT_EQUAL(implementation.utf8_length_from_latin1(p, n),
                   simdutf::scalar::latin1::utf8_length_from_latin1(p, n));
      ASSERT_TRUE(implementation.validate_utf8(p, n));
      ASSERT_TRUE(same(implementation.validate_utf8_with_counts(p, n),
                       simdutf::scalar::utf8::validate_with_counts(p, n)));
      for (size_t pos : positions<char>(n, off, n)) {
        for (uint8_t b : bad) {
          std::vector<char> v = valid;
          v[pos] = char(b);
          const char *q = buf.at(off, v);
          ASSERT_EQUAL(implementation.validate_utf8(q, n),
                       simdutf::scalar::utf8::validate(q, n));
          ASSERT_TRUE(same(implementation.validate_utf8_with_errors(q, n),
                           simdutf::scalar::utf8::validate_with_errors(q, n)));
          ASSERT_TRUE(same(implementation.validate_utf8_with_counts(q, n),
                           simdutf::scalar::utf8::validate_with_counts(q, n)));
        }
        std::vector<char> v = ascii;
        v[pos] = char(0x80);
        const char *q = buf.at(off, v);
        ASSERT_EQUAL(implementation.validate_ascii(q, n),
                     simdutf::scalar::ascii::validate(q, n));
        ASSERT_TRUE(same(implementation.validate_ascii_with_errors(q, n),
                         simdutf::scalar::ascii::validate_with_errors(q, n)));
      }
      const char *q = buf.at(off, ascii);
      ASSERT_TRUE(implementation.validate_ascii(q, n));
      ASSERT_TRUE(same(implementation.validate_ascii_with_errors(q, n),
                       simdutf::scalar::ascii::validate_with_errors(q, n)));
    }
  }
}

template <simdutf::endianness E>
void check_utf16(const simdutf::implementation &implementation) {
  constexpr bool be = (E == simdutf::endianness::BIG);
  const auto native = [](char16_t c) {
    return simdutf::scalar::utf16::swap_if_needed<E>(c);
  };
  std::mt19937 gen(1234);
  const char16_t bad[] = {0xD800, 0xDBFF, 0xDC00, 0xDFFF, u'a'};
  for (size_t bytes : sizes_in_bytes) {
    const size_t n = bytes / 2;
    const std::vector<char16_t> valid = make_utf16(gen, n, be);
    std::vector<char16_t> ascii(n);
    for (auto &c : ascii) {
      c = native(char16_t(std::uniform_int_distribution<int>(0, 0x7F)(gen)));
    }
    aligned_buffer<char16_t> buf(n);
    std::vector<char16_t> out(n), expected(n);
    for (size_t off = 0; off < 64; off += 2) {
      const char16_t *p = buf.at(off, valid);
      ASSERT_EQUAL(be ? implementation.count_utf16be(p, n)
                      : implementation.count_utf16le(p, n),
                   simdutf::scalar::utf16::count_code_points<E>(p, n));
      ASSERT_EQUAL(be ? implementation.utf8_length_from_utf16be(p, n)
                      : implementation.utf8_length_from_utf16le(p, n),
                   simdutf::scalar::utf16::utf8_length_from_utf16<E>(p, n));
      ASSERT_EQUAL(be ? implementation.utf32_length_from_utf16be(p, n)
                      : implementation.utf32_length_from_utf16le(p, n),
                   simdutf::scalar::utf16::utf32_length_from_utf16<E>(p, n));
      for (size_t pos : positions<char16_t>(bytes, off, n)) {
        for (char16_t b : bad) {
          std::vector<char16_t> v = valid;
          v[pos] = native(b);
          const char16_t *q = buf.at(off, v);
          ASSERT_EQUAL(be ? implementation.validate_utf16be(q, n)
                          : implementation.validate_utf16le(q, n),
                       simdutf::scalar::utf16::validate<E>(q, n));
          ASSERT_TRUE(
              same(be ? implementation.validate_utf16be_with_errors(q, n)
                      : implementation.validate_utf16le_with_errors(q, n),
                   simdutf::scalar::utf16::validate_with_errors<E>(q, n)));
          ASSERT_TRUE(same(
              be ? implementation.utf8_length_from_utf16be_with_replacement(q,
                                                                            n)
                 : implementation.utf8_length_from_utf16le_with_replacement(q,
                                                                            n),
              simdutf::scalar::utf16::utf8_length_from_utf16_with_replacement<
                  E>(q, n)));
          if (be) {
            implementation.to_well_formed_utf16be(q, n, out.data());
          } else {
            implementation.to_well_formed_utf16le(q, n, out.data());
          }
          simdutf::scalar::utf16::to_well_formed_utf16<E>(q, n,
                                                          expected.data());
          ASSERT_TRUE(out == expected);
        }
        std::vector<char16_t> v = ascii;
        v[pos] = native(0x80);
        const char16_t *q = buf.at(off, v);
        ASSERT_EQUAL(be ? implementation.validate_utf16be_as_ascii(q, n)
                        : implementation.validate_utf16le_as_ascii(q, n),
                     simdutf::scalar::utf16::validate_as_ascii<E>(q, n));
      }
    }
  }
}

TEST(utf16le_functions_at_every_offset) {
  check_utf16<simdutf::endianness::LITTLE>(implementation);
}

TEST(utf16be_functions_at_every_offset) {
  check_utf16<simdutf::endianness::BIG>(implementation);
}

TEST(utf32_functions_at_every_offset) {
  std::mt19937 gen(1234);
  const char32_t bad[] = {0xD800, 0xDFFF, 0x110000, 0xFFFFFFFF};
  for (size_t bytes : sizes_in_bytes) {
    const size_t n = bytes / 4;
    const std::vector<char32_t> valid = make_utf32(gen, n);
    aligned_buffer<char32_t> buf(n);
    for (size_t off = 0; off < 64; off += 4) {
      const char32_t *p = buf.at(off, valid);
      ASSERT_EQUAL(implementation.utf8_length_from_utf32(p, n),
                   simdutf::scalar::utf32::utf8_length_from_utf32(p, n));
      ASSERT_EQUAL(implementation.utf16_length_from_utf32(p, n),
                   simdutf::scalar::utf32::utf16_length_from_utf32(p, n));
      for (size_t pos : positions<char32_t>(bytes, off, n)) {
        for (char32_t b : bad) {
          std::vector<char32_t> v = valid;
          v[pos] = b;
          const char32_t *q = buf.at(off, v);
          ASSERT_EQUAL(implementation.validate_utf32(q, n),
                       simdutf::scalar::utf32::validate(q, n));
          ASSERT_TRUE(same(implementation.validate_utf32_with_errors(q, n),
                           simdutf::scalar::utf32::validate_with_errors(q, n)));
        }
      }
    }
  }
}

TEST_MAIN
