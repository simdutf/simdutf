#include "simdutf.h"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <iomanip>

int main() {
  printf("running hard-coded ASCII tests... ");
  fflush(NULL);
  // additional tests are from autobahn websocket testsuite
  // https://github.com/crossbario/autobahn-testsuite/tree/master/autobahntestsuite/autobahntestsuite/case
  const char *goodsequences[] = {"a","b","ascsacdsasdsadsadsadasdasdsads", "\x71", "\x75", "\x7f"};
  const char *badsequences[] = {"\xff", "a"};
  for (size_t i = 0; i < sizeof(goodsequences)/sizeof(goodsequences[0]); i++) {
    size_t len = std::strlen(goodsequences[i]);
    if (!simdutf::validate_ascii(goodsequences[i], len)) {
      printf("bug goodsequences[%zu]\n", i);
      abort();
    }
  }
  for (size_t i = 0; i < sizeof(badsequences)/sizeof(badsequences[0]); i++) {
    size_t len = std::strlen(badsequences[i]);
    if (simdutf::validate_ascii(badsequences[i], len)) {
      printf("bug lookup2 badsequences[%zu]\n", i);
      abort();
    }
  }

  puts("OK");

  return EXIT_SUCCESS;
}