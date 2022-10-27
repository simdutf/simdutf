#include "simdutf.h"
#include <cstddef>
#include <cstdint>
#include <random>
#include <iostream>
#include <iomanip>

// This is an attempt at reproducing an issue with the utf8 fuzzer
int main() {
  std::cout << "running puzzler... " << std::endl;
  const char* bad64 = "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x1c\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";
  size_t length = 64;
  std::cout << "Input: \"";
  for(size_t j = 0; j < length; j++) {
    std::cout << "\\x" << std::hex << std::setw(2) << std::setfill('0') << uint32_t(bad64[j]);
  }
  std::cout << "\"" << std::endl;
  bool is_ok{true};
  for(const auto& e: simdutf::available_implementations) {
      if(!e->supported_by_runtime_system()) {
        std::cout << e->name() << " is unsupported by current processor " << std::endl;
        continue;
      }
      const bool current = e->validate_utf8(bad64, length);
      std::cout << e->name() << " returns " << current << std::endl;
      if(current) { is_ok = false; }
  }
  if(!is_ok) {
    puts("FAIL");
    EXIT_FAILURE;
  }

  puts("OK");

  return EXIT_SUCCESS;
}
