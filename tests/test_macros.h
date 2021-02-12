#pragma once

namespace {

  using test_procedure = void (*)(const simdutf::implementation& impl);
  std::list<test_procedure>& test_procedures() {
    static std::list<test_procedure> singleton;

    return singleton;
  }

  struct register_test {
    register_test(test_procedure proc) {
      test_procedures().push_back(proc);
    }
  };

}

#define TEST(name)                                          \
void test_impl_##name(const simdutf::implementation& impl); \
void name(const simdutf::implementation& impl) {            \
  std::string title = #name;                                \
  std::replace(title.begin(), title.end(), '_', ' ');       \
  printf("%s...", title.c_str()); fflush(stdout);           \
  test_impl_##name(impl);                                   \
  puts(" OK");                                              \
}                                                           \
static register_test test_register_##name(name);            \
void test_impl_##name(const simdutf::implementation& implementation)

#define ASSERT_TRUE(cond) {                                 \
  const bool expr = (cond);                                 \
  if (!expr) {                                              \
    printf("expected %s to be true, it's false\n", #cond);  \
    exit(1);                                                \
  }                                                         \
}

#define ASSERT_FALSE(cond) {                                \
  const bool expr = !(cond);                                \
  if (!expr) {                                              \
    printf("expected %s to be false, it's true\n", #cond);  \
    exit(1);                                                \
  }                                                         \
}
