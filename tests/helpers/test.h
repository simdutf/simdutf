#pragma once

#include "simdutf.h"
#include <algorithm>
#include <string>
#include <list>
#include <iostream>


namespace simdutf { namespace test {

  int main(int argc, char* argv[]);
  using test_procedure = void (*)(const simdutf::implementation& impl);
  struct test_entry {
    std::string name;
    test_procedure procedure;

    void operator()(const simdutf::implementation& impl) {
      procedure(impl);
    }
  };

  std::list<test_entry>& test_procedures();

  struct register_test {
    register_test(const char* name, test_procedure proc);
  };

}} // namespace namespace simdutf::test


#define TEST(name)                                          \
void test_impl_##name(const simdutf::implementation& impl); \
void name(const simdutf::implementation& impl) {            \
  std::string title = #name;                                \
  std::replace(title.begin(), title.end(), '_', ' ');       \
  printf("Running '%s'... ", title.c_str()); fflush(stdout);\
  test_impl_##name(impl);                                   \
  puts(" OK");                                              \
}                                                           \
static simdutf::test::register_test test_register_##name(#name, name); \
void test_impl_##name(const simdutf::implementation& implementation)

#define ASSERT_EQUAL(a, b) {                                                   \
  const auto expr = (a);                                                       \
  if (expr != b) {                                                             \
    std::cout << "\nExpected " << expr << " to be " << b << ".\n";             \
    printf("%s \n",#a);                                                        \
    printf("file %s:%d, function %s  \n", __FILE__, __LINE__, __func__); \
    exit(1);                                                      \
  }                                                               \
}

#define ASSERT_TRUE(cond) {                                 \
  const bool expr = (cond);                                 \
  if (!expr) {                                              \
    printf("expected %s to be true, it's false\n", #cond);  \
    printf("file %s:%d, function %s  \n", __FILE__, __LINE__, __func__); \
    exit(1);                                                \
  }                                                         \
}

#define ASSERT_FALSE(cond) {                                \
  const bool expr = !(cond);                                \
  if (!expr) {                                              \
    printf("expected %s to be false, it's true\n", #cond);  \
    printf("file %s:%d, function %s  \n", __FILE__, __LINE__, __func__); \
    exit(1);                                                \
  }                                                         \
}
