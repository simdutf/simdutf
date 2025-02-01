#include "simdutf.h"

#if SIMDUTF_CPLUSPLUS20
  #include <barrier>
#endif
#include <random>
#include <thread>
#include <vector>

#include <tests/helpers/test.h>

#if !defined(SIMDUTF_NO_THREADS) && SIMDUTF_ATOMIC_REF && SIMDUTF_SPAN

TEST(empty_input_gives_empty_output) {
  std::vector<char> input;
  std::vector<char> output;

  const auto ret = simdutf::atomic_binary_to_base64(input, output);
  ASSERT_EQUAL(ret, 0);
}

TEST(small_input) {
  const std::string input{"Abracadabra!"};
  const std::string expected_output{"QWJyYWNhZGFicmEh"};
  std::string output(expected_output.size(), '\0');

  const auto ret = simdutf::atomic_binary_to_base64(input, output);
  ASSERT_EQUAL(ret, expected_output.size());
  ASSERT_EQUAL(output, expected_output);
}

TEST(large_input) {
  // large means larger than one internal block of data
  const std::size_t N_input = 1'000'000;
  std::string input, expected_output;
  for (std::size_t i = 0; i < N_input; ++i) {
    input.append("abc");
    expected_output.append("YWJj");
  }
  std::string output(expected_output.size(), '\0');
  const auto ret = simdutf::atomic_binary_to_base64(input, output);
  ASSERT_EQUAL(ret, expected_output.size());
  ASSERT_EQUAL(output, expected_output);
}

  #if SIMDUTF_CPLUSPLUS20

TEST(threaded) {
  // large means larger than one internal block of data
  const std::size_t N_input = 1'000'000;
  std::string input, expected_output;
  for (std::size_t i = 0; i < N_input; ++i) {
    input.append("abc");
    expected_output.append("YWJj");
  }
  std::string output(expected_output.size(), '\0');

  constexpr auto nthreads = 4;

  std::barrier sync_point(nthreads + 1, [nthreads]() noexcept {
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
        std::atomic_ref(input[input_index_dist(gen)]).fetch_add(dist(gen));
        std::atomic_ref(output[output_index_dist(gen)]).store(dist(gen));
      }
    });
  }

  sync_point.arrive_and_wait();

  for (int i = 0; i < 10; ++i) {
    // don't bother to look at the output, it can fail or succeed depending
    // on the thread scheduling
    [[maybe_unused]] const auto ret =
        simdutf::atomic_binary_to_base64(input, output);
  }

  keep_running = false;
}
  #endif // SIMDUTF_CPLUSPLUS20

#endif // #if !defined(SIMDUTF_NO_THREADS) && SIMDUTF_ATOMIC_REF && SIMDUTF_SPAN

TEST_MAIN
