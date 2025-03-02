#include "simdutf.h"

#include <tests/helpers/test.h>

int main(int argc, char *argv[]) {
#ifdef SIMDUTF_INTERNAL_TESTS
  bool any_added = false;
  for (const auto &implementation : simdutf::get_available_implementations()) {
    for (const auto &test : implementation->internal_tests()) {
      simdutf::test::test_procedures().push_back(simdutf::test::test_entry{
          test.name,
          test.procedure,
      });
      any_added = true;
    }
  }

  if (not any_added) {
    puts("None of implementations provides internal tests, skipping.");
    return 0;
  }

  const auto cmdline = simdutf::test::CommandLine::parse(argc, argv);
  simdutf::test::run(cmdline);
#endif
  return 0;
}
