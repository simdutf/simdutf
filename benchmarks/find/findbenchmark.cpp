#include <algorithm>
#include <string>
#include <iostream>
#include <chrono>
#include <random>

#include "findbenchmarker.h"

#include "simdutf.h"

void pretty_print(const std::string &name, size_t num_values,
                  event_aggregate agg) {
  printf("%-50s : ", name.c_str());
  printf(" %5.2f ns ", agg.elapsed_ns());
  if (collector.has_events()) {
    printf(" %5.2f GHz ", agg.cycles() / agg.elapsed_ns());
    printf(" %5.2f c ", agg.cycles() / num_values);
    printf(" %5.2f i ", agg.instructions() / num_values);
    printf(" %5.2f i/c ", agg.instructions() / agg.cycles());
  }
  printf("\n");
}

std::string generate_random_ascii_string(size_t length) {
  const char charset[] =
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  const size_t max_index = sizeof(charset) - 1;
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, max_index - 1);

  std::string result;
  result.reserve(length);
  for (size_t i = 0; i < length; ++i) {
    result += charset[dis(gen)];
  }
  result += '='; // Add '=' at the end
  return result;
}

int main(int argc, char **argv) {
  std::string input = generate_random_ascii_string(10000);
  size_t volume = input.size();
  volatile uint64_t counter = 0;
  for (size_t i = 0; i < 4; i++) {
    printf("Run %zu\n", i + 1);
    pretty_print("std::find", input.size(), bench([&input, &counter]() {
                   auto it = std::find(input.data(),
                                       input.data() + input.size(), '=');
                   counter = counter + size_t(it - input.data());
                 }));

    pretty_print("simdutf::find", input.size(), bench([&input, &counter]() {
                   auto it = simdutf::find(input.data(),
                                           input.data() + input.size(), '=');
                   counter = counter + size_t(it - input.data());
                 }));
  }
}
