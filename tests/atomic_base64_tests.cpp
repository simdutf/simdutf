#include "simdutf.h"

#if SIMDUTF_CPLUSPLUS20
  #include <barrier>
#endif
#include <memory>
#include <random>
#include <thread>
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

TEST(threaded) {
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

  constexpr auto nthreads = 4;

  std::barrier sync_point(nthreads + 1, [&]() noexcept {
    std::printf("all %u threads reached the barrier\n", nthreads);
  });

  std::vector<std::jthread> threads;
  std::atomic_bool keep_running{true};
  for (std::size_t threadi = 0; threadi < nthreads; ++threadi) {
    threads.emplace_back([&, threadi]() {
      std::mt19937 gen{static_cast<std::mt19937::result_type>(threadi)};
      std::uniform_int_distribution<int> dist{0, 255};
      std::uniform_int_distribution<std::size_t> input_index_dist{
          0, input.size() - 1};
      std::uniform_int_distribution<std::size_t> output_index_dist{
          0, expected_output.size() - 1};
      sync_point.arrive_and_wait();
      while (keep_running) {
        input[input_index_dist(gen)].fetch_add(dist(gen));
        output.get()[output_index_dist(gen)] = dist(gen);
      }
    });
  }

  sync_point.arrive_and_wait();

  for (int i = 0; i < 10; ++i) {
    // don't bother to look at the output, it can fail or succeed depending
    // on the thread scheduling
    [[maybe_unused]] const auto ret = simdutf::binary_to_base64(
        input, std::span(output.get(), expected_output.size()));
  }

  keep_running = false;
}

#endif
TEST_MAIN
