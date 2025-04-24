#include "simdutf.h"

#if SIMDUTF_CPLUSPLUS20
  #include <barrier>
#endif
#include <random>
#include <thread>
#include <vector>

#include <tests/helpers/test.h>

// check if we are running with thread sanitizer
#if defined(__clang__)
  #if __has_feature(thread_sanitizer)
    #define RUNNING_UNDER_THREAD_SANITIZER 1
  #else
    #define RUNNING_UNDER_THREAD_SANITIZER 0
  #endif
#elif defined(__GNUC__)
  #if defined(__SANITIZE_THREAD__)
    #define RUNNING_UNDER_THREAD_SANITIZER 1
  #else
    #define RUNNING_UNDER_THREAD_SANITIZER 0
  #endif
#else
  #define RUNNING_UNDER_THREAD_SANITIZER 0
#endif

#if !defined(SIMDUTF_NO_THREADS) && SIMDUTF_ATOMIC_REF && SIMDUTF_SPAN

TEST(empty_input_gives_empty_output) {
  std::vector<char> input;
  std::vector<char> output;

  {
    const auto ret = simdutf::atomic_binary_to_base64(input, output);
    ASSERT_EQUAL(ret, 0);
  }
  {
    const auto s = std::span(output);
    const auto [ret, outlen] = simdutf::atomic_base64_to_binary_safe(input, s);
    ASSERT_EQUAL(ret.error, simdutf::SUCCESS);
    ASSERT_EQUAL(outlen, 0);
  }
  {
    const auto [ret, outlen] =
        simdutf::atomic_base64_to_binary_safe(input, std::span(output));
    ASSERT_EQUAL(ret.error, simdutf::SUCCESS);
    ASSERT_EQUAL(outlen, 0);
  }
  {
    const auto [ret, outlen] =
        simdutf::atomic_base64_to_binary_safe(input, output);
    ASSERT_EQUAL(ret.error, simdutf::SUCCESS);
    ASSERT_EQUAL(outlen, 0);
  }
}

TEST(small_input) {
  const std::string input{"Abracadabra!"};
  const std::string expected_output{"QWJyYWNhZGFicmEh"};
  std::string output(expected_output.size(), '\0');

  const auto ret = simdutf::atomic_binary_to_base64(input, output);
  ASSERT_EQUAL(ret, expected_output.size());
  ASSERT_EQUAL(output, expected_output);

  // try to recover the input by going in the opposite direction
  std::string recovered;
  {
    // try with a (too) small buffer
    const auto [ret2, outlen] = simdutf::atomic_base64_to_binary_safe(
        std::span(output).first(expected_output.size()), recovered);
    ASSERT_EQUAL(ret2.error, simdutf::OUTPUT_BUFFER_TOO_SMALL);
    ASSERT_EQUAL(outlen, 0);
  }
  {
    // ...then with a larger size
    recovered.resize(100);
    const auto [ret2, outlen] = simdutf::atomic_base64_to_binary_safe(
        std::span(output).first(expected_output.size()), recovered);
    ASSERT_EQUAL(ret2.error, simdutf::SUCCESS);
    ASSERT_EQUAL(outlen, input.size());
    recovered.resize(outlen);
  }
  ASSERT_EQUAL(input, recovered);
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

  // try to recover the input by going in the opposite direction
  std::string recovered(input.size(), '?');
  const auto [ret2, outlen] = simdutf::atomic_base64_to_binary_safe(
      std::span(output).first(ret), recovered);
  ASSERT_EQUAL(ret2.error, simdutf::SUCCESS);
  ASSERT_EQUAL(outlen, input.size());
  recovered.resize(outlen);
  ASSERT_EQUAL(input, recovered);
}

void varying_input_size_utf16_impl(const std::size_t N_input) {
  std::string input;
  std::u16string expected_output;
  for (std::size_t i = 0; i < N_input; ++i) {
    input.append("abc");
    expected_output.append(u"YWJj");
  }
  std::u16string output16(expected_output.size(), '\0');
  {
    std::string output(expected_output.size(), '\0');
    const auto ret = simdutf::atomic_binary_to_base64(input, output);
    ASSERT_EQUAL(output.size(),
                 simdutf::convert_utf8_to_utf16le(output, output16));
    ASSERT_EQUAL(ret, expected_output.size());
    ASSERT_TRUE(output16 == expected_output);
  }

  // try to recover the input by going in the opposite direction
  std::string recovered(input.size(), '?');
  const auto [ret2, outlen] =
      simdutf::atomic_base64_to_binary_safe(std::span(output16), recovered);
  ASSERT_EQUAL(ret2.error, simdutf::SUCCESS);
  ASSERT_EQUAL(outlen, input.size());
  recovered.resize(outlen);

  ASSERT_EQUAL(input, recovered);
}
TEST(varying_input_size_utf16) {
  varying_input_size_utf16_impl(0);
  varying_input_size_utf16_impl(1);

  varying_input_size_utf16_impl(1'000'000);
}
  #if SIMDUTF_CPLUSPLUS20

    #if RUNNING_UNDER_THREAD_SANITIZER
// this test is only relevant if compiling with thread sanitizer
TEST(threaded_binary_to_base64) {
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
      sync_point.arrive_and_wait();
      while (keep_running) {
        std::atomic_ref(input[input_index_dist(gen)]).fetch_add(dist(gen));
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

TEST(threaded_base64_to_binary_safe) {
  // large means larger than one internal block of data
  const std::size_t N_input = 1'000'000;
  std::string input, expected_output;
  for (std::size_t i = 0; i < N_input; ++i) {
    input.append("YWJj");
    expected_output.append("abc");
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
      std::uniform_int_distribution<std::size_t> output_index_dist{
          0, output.size() - 1};
      sync_point.arrive_and_wait();
      while (keep_running) {
        std::atomic_ref(output[output_index_dist(gen)]).fetch_add(dist(gen));
      }
    });
  }

  sync_point.arrive_and_wait();

  for (int i = 0; i < 10; ++i) {
    // don't bother to look at the output, it can fail or succeed depending
    // on the thread scheduling
    [[maybe_unused]] const auto ret =
        simdutf::atomic_base64_to_binary_safe(input, output);
  }

  keep_running = false;
}
    #endif // RUNNING_UNDER_THREAD_SANITIZER
  #endif   // SIMDUTF_CPLUSPLUS20

#endif // #if !defined(SIMDUTF_NO_THREADS) && SIMDUTF_ATOMIC_REF &&
       // SIMDUTF_SPAN

TEST_MAIN
