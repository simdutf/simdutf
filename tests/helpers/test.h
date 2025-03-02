#pragma once

#include "simdutf.h"
#include <algorithm>
#include <sstream>
#include <string>
#include <list>
#include <set>
#include <vector>

namespace simdutf {
namespace test {

struct CommandLine {
  bool show_help{false};
  bool show_tests{false};
  bool show_architectures{false};
  std::set<std::string> architectures;
  std::vector<std::string> tests;
  uint64_t seed;

  static CommandLine parse(int argc, char *argv[]);
};

int main(int argc, char *argv[]);
void run(const CommandLine &cmdline);

using test_procedure = void (*)(const simdutf::implementation &impl);
struct test_entry {
  std::string name;
  test_procedure procedure;

  void operator()(const simdutf::implementation &impl);
};

std::list<test_entry> &test_procedures();

struct register_test {
  register_test(const char *name, test_procedure proc);
};

} // namespace test
} // namespace simdutf

template <typename T> void dump_ascii(const T &values) {
  for (size_t i = 0; i < values.size(); i++) {
    const uint8_t b = values[i];
    if (b >= 32 and b < 128) {
      printf(" %c ", b);
    } else {
      printf("   ");
    }
  }
  putchar('\n');
}

template <typename T> void dump_hex(const T &values) {
  for (size_t i = 0; i < values.size(); i++) {
    const uint8_t b = values[i];
    printf(" %02x", b);
  }
  putchar('\n');
}

template <typename T, typename U>
void dump_diff_hex(const T &lhs, const U &rhs) {
  const size_t ls = lhs.size();
  const size_t rs = rhs.size();
  const size_t size = (ls <= rs) ? ls : rs;
  for (size_t i = 0; i < size; i++) {
    const uint8_t l = lhs[i];
    const uint8_t r = rhs[i];
    if (l != r) {
      printf(" %02x", l);
    } else {
      printf("   ");
    }
  }
  putchar('\n');
}

#define TEST(name)                                                             \
  void test_impl_##name(const simdutf::implementation &impl);                  \
  void name(const simdutf::implementation &impl) {                             \
    simdutf::get_active_implementation() = &impl;                              \
    test_impl_##name(impl);                                                    \
  }                                                                            \
  static simdutf::test::register_test test_register_##name(#name, name);       \
  void test_impl_##name(                                                       \
      [[maybe_unused]] const simdutf::implementation &implementation)

#define TEST_LOOP(trials, name)                                                \
  void test_impl_##name(const simdutf::implementation &impl, uint32_t seed);   \
  void name(const simdutf::implementation &impl) {                             \
    for (size_t trial = 0; trial < (trials); trial++) {                        \
      const uint32_t seed{1234 + uint32_t(trial)};                             \
      if ((trial % 100) == 0) {                                                \
        putchar('.');                                                          \
        fflush(stdout);                                                        \
      }                                                                        \
      test_impl_##name(impl, seed);                                            \
    }                                                                          \
  }                                                                            \
  static simdutf::test::register_test test_register_##name(#name, name);       \
  void test_impl_##name(const simdutf::implementation &implementation,         \
                        [[maybe_unused]] uint32_t seed)

#define ASSERT_EQUAL(a, b)                                                     \
  {                                                                            \
    const auto lhs = (a);                                                      \
    const auto rhs = (b);                                                      \
    if (lhs != rhs) {                                                          \
      std::stringstream lhs_str;                                               \
      lhs_str << lhs;                                                          \
      std::stringstream rhs_str;                                               \
      rhs_str << rhs;                                                          \
      printf("lhs: %s = %s\n", #a, lhs_str.str().c_str());                     \
      printf("rhs: %s = %s\n", #b, rhs_str.str().c_str());                     \
      printf("%s \n", #a);                                                     \
      printf("file %s:%d, function %s  \n", __FILE__, __LINE__, __func__);     \
      exit(1);                                                                 \
    }                                                                          \
  }

#define ASSERT_BYTES_EQUAL(a, b, len)                                          \
  {                                                                            \
    const auto lhs = (a);                                                      \
    const auto rhs = (b);                                                      \
    if (!std::equal(lhs.begin(), lhs.begin() + len, rhs.begin())) {            \
      printf("lhs = `%s`\n", #a);                                              \
      printf(" ascii: ");                                                      \
      dump_ascii(lhs);                                                         \
      printf("   hex: ");                                                      \
      dump_hex(lhs);                                                           \
      printf("rhs = `%s`\n", #b);                                              \
      printf(" ascii: ");                                                      \
      dump_ascii(rhs);                                                         \
      printf("   hex: ");                                                      \
      dump_hex(rhs);                                                           \
      printf("  diff: ");                                                      \
      dump_diff_hex(lhs, rhs);                                                 \
      printf("file %s:%d, function %s\n", __FILE__, __LINE__, __func__);       \
      exit(1);                                                                 \
    }                                                                          \
  }

#define ASSERT_TRUE(cond)                                                      \
  {                                                                            \
    const bool expr = (cond);                                                  \
    if (!expr) {                                                               \
      printf("expected %s to be true, it's false\n", #cond);                   \
      printf("file %s:%d, function %s  \n", __FILE__, __LINE__, __func__);     \
      exit(1);                                                                 \
    }                                                                          \
  }

#define ASSERT_FALSE(cond)                                                     \
  {                                                                            \
    const bool expr = !(cond);                                                 \
    if (!expr) {                                                               \
      printf("expected %s to be false, it's true\n", #cond);                   \
      printf("file %s:%d, function %s  \n", __FILE__, __LINE__, __func__);     \
      exit(1);                                                                 \
    }                                                                          \
  }

#define TEST_MAIN                                                              \
  int main(int argc, char *argv[]) { return simdutf::test::main(argc, argv); }
