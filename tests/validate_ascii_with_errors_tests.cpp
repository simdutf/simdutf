#include "simdutf.h"

#include <array>
#include <algorithm>

#include <tests/helpers/random_utf8.h>
#include <tests/helpers/test.h>
#include <fstream>
#include <memory>

TEST(no_error_ASCII) {
    uint32_t seed{1234};
    simdutf::tests::helpers::random_utf8 generator{seed, 1, 0, 0, 0};

    for(size_t trial = 0; trial < 1000; trial++) {
        const auto ascii{generator.generate(512)};

        simdutf::result res = implementation.validate_ascii_with_errors(reinterpret_cast<const char*>(ascii.data()), ascii.size());

        ASSERT_EQUAL(res.error, simdutf::error_code::SUCCESS);
        ASSERT_EQUAL(res.count, ascii.size());
    }
}

TEST(error_ASCII) {
    uint32_t seed{1234};
    simdutf::tests::helpers::random_utf8 generator{seed, 1, 0, 0, 0};

    for(size_t trial = 0; trial < 1000; trial++) {
        auto ascii{generator.generate(512)};

        for (int i = 0; i < ascii.size(); i++) {
            ascii[i] += 0b10000000;

            simdutf::result res = implementation.validate_ascii_with_errors(reinterpret_cast<const char*>(ascii.data()), ascii.size());

            ASSERT_EQUAL(res.error, simdutf::error_code::TOO_LARGE);
            ASSERT_EQUAL(res.count, i);

            ascii[i] -= 0b10000000;
        }
    }
}

int main(int argc, char* argv[]) {
  return simdutf::test::main(argc, argv);
}
