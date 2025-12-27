#include "simdutf.h"

#include <tests/helpers/fixed_string.h>
#include <tests/helpers/random_utf8.h>
#include <tests/helpers/test.h>

TEST_LOOP(no_error_ASCII) {
  simdutf::tests::helpers::random_utf8 generator{seed, 1, 0, 0, 0};
  const auto ascii{generator.generate(512)};

  simdutf::result res = implementation.validate_ascii_with_errors(
      reinterpret_cast<const char *>(ascii.data()), ascii.size());

  ASSERT_EQUAL(res.error, simdutf::error_code::SUCCESS);
  ASSERT_EQUAL(res.count, ascii.size());
}

TEST_LOOP(error_ASCII) {
  simdutf::tests::helpers::random_utf8 generator{seed, 1, 0, 0, 0};

  auto ascii{generator.generate(512)};

  for (unsigned int i = 0; i < ascii.size(); i++) {
    ascii[i] += 0b10000000;

    simdutf::result res = implementation.validate_ascii_with_errors(
        reinterpret_cast<const char *>(ascii.data()), ascii.size());

    ASSERT_EQUAL(res.error, simdutf::error_code::TOO_LARGE);
    ASSERT_EQUAL(res.count, i);

    ascii[i] -= 0b10000000;
  }
}

#if SIMDUTF_CPLUSPLUS23

namespace {
// for negative compilation tests
template <class InputPtr>
concept passable_to_validate = requires(InputPtr p) {
  simdutf::scalar::ascii::validate_with_errors(p, 10u);
};

} // namespace

TEST(compile_time_valid) {

  using namespace simdutf::tests::helpers;
  constexpr auto ascii = "a normal ascii text"_latin1;

  static_assert(simdutf::validate_ascii_with_errors(ascii).is_ok());
  static_assert(
      simdutf::validate_ascii_with_errors(ascii.as_array<unsigned char>())
          .is_ok());
  static_assert(
      simdutf::validate_ascii_with_errors(ascii.as_array<signed char>())
          .is_ok());
  static_assert(
      simdutf::validate_ascii_with_errors(ascii.as_array<std::byte>()).is_ok());

  static_assert(passable_to_validate<char *>);
  static_assert(passable_to_validate<unsigned char *>);
  static_assert(passable_to_validate<const char *>);
  static_assert(!passable_to_validate<int *>);
  static_assert(passable_to_validate<std::array<char, 10>>);
  static_assert(!passable_to_validate<std::array<int, 10>>);
}

TEST(compile_time_invalid) {
  using namespace simdutf::tests::helpers;
  constexpr auto not_ascii = u8"not ascii: köttbulle"_utf8;
  static_assert(simdutf::validate_ascii_with_errors(not_ascii).is_err());
}
#endif

TEST_MAIN
