#include "simdutf.h"

#include <memory>
#include <vector>

#include <tests/helpers/test.h>

#if SIMDUTF_SPAN
  #include <span>
TEST(empty_input_gives_empty_output) {
  std::vector<std::atomic<char>> input;
  std::vector<std::atomic<char>> output;

  const auto ret = simdutf::binary_to_base64(input, output);
  ASSERT_EQUAL(ret, 0);
}

TEST(easy_test_case) {
  const std::string non_atomic_input{"Abracadabra!"};
  const std::string expected_output{"QWJyYWNhZGFicmEh"};
  std::vector<std::atomic<char>> input{begin(non_atomic_input),
                                       end(non_atomic_input)};
  // std::atomic is noncopyable and nonmovable
  std::unique_ptr<std::atomic<char>[]> output(
      new std::atomic<char>[expected_output.size()]);

  const auto ret = simdutf::binary_to_base64(
      input, std::span(output.get(), expected_output.size()));
  ASSERT_EQUAL(ret, expected_output.size());
  const std::string actual_output{output.get(),
                                  output.get() + expected_output.size()};
  ASSERT_EQUAL(actual_output, expected_output);
}

TEST(large_input) {
  // large means larger than one internal block of data
  const std::size_t N_input = 1'000'000;
  std::string non_atomic_input, expected_output;
  for (std::size_t i = 0; i < N_input; ++i) {
    non_atomic_input.append("abc");
    expected_output.append("YWJj");
  }
  std::vector<std::atomic<char>> input{begin(non_atomic_input),
                                       end(non_atomic_input)};
  // std::atomic is noncopyable and nonmovable
  std::unique_ptr<std::atomic<char>[]> output(
      new std::atomic<char>[expected_output.size()]);

  const auto ret = simdutf::binary_to_base64(
      input, std::span(output.get(), expected_output.size()));
  ASSERT_EQUAL(ret, expected_output.size());
  const std::string actual_output{output.get(),
                                  output.get() + expected_output.size()};
  ASSERT_EQUAL(actual_output, expected_output);
}

#endif
TEST_MAIN
