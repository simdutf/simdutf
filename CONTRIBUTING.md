# Contributing to simdutf

Thank you for your interest in contributing to **simdutf**, a high-performance library for UTF-8, UTF-16, and UTF-32 transcoding as well as other formats such as base64 using SIMD instructions. We welcome contributions to enhance the project’s speed, reliability, and usability. This guide outlines how to contribute effectively.

## Table of Contents
- [Ways to Contribute](#ways-to-contribute)
  - [Reporting Bugs](#reporting-bugs)
  - [Suggesting Features](#suggesting-features)
  - [Submitting Pull Requests](#submitting-pull-requests)
- [Development Setup](#development-setup)
- [Coding Guidelines](#coding-guidelines)
- [Testing](#testing)
- [Documentation](#documentation)
- [Licensing](#licensing)


## Ways to Contribute

You can contribute by reporting bugs, suggesting features, improving documentation, or submitting code changes.

### Reporting Bugs

To report a bug, open an issue on the [GitHub Issues page](https://github.com/simdutf/simdutf/issues) and include:
- A clear title and description.
- Steps to reproduce, expected vs. actual behavior.
- Environment details (OS, compiler, CPU architecture).
- Relevant logs or code snippets.

We prefer that you use our bug template.

### Suggesting Features

To propose a feature:
1. Check existing issues to avoid duplicates.
2. Open an issue.
3. Describe the feature, its benefits, and potential implementation details.

### Submitting Pull Requests

To contribute code or documentation:
1. Fork the [simdutf repository](https://github.com/simdutf/simdutf).
2. Clone your fork:
    ```bash
    git clone https://github.com/your-username/simdutf.git
    ```
3. Create a branch:
    ```bash
    git checkout -b feature/your-feature-name
    ```
4. Make changes, following Coding Guidelines (#coding-guidelines).
5. Run tests (see Testing (#testing)).
6. Commit with a clear message:
    ```bash
    git commit -m "Add feature: brief description"
    ```
7. Push to your fork:
    ```bash
    git push origin feature/your-feature-name
    ```
8. Open a pull request with: A descriptive title and explanation. References to related issues (e.g., "Fixes #123"). 
9. Address feedback during review.

## Development Setup

To set up your environment:

Requirements:
- We recommend a C++20-compatible compiler (e.g., GCC, Clang, MSVC). Although the library can be built with a C++11 compatible compiler, some of our tools and functionality requires C++20.
- A recent version of CMake.
- Git.
- Optional: SIMD support (SSE, AVX, NEON) for your platform.

Clone the repository:
```bash
git clone https://github.com/simdutf/simdutf.git
cd simdutf
````

Build:
```bash
cmake -B build  -DSIMDUTF_CXX_STANDARD=20
cmake --build build
```

Under Visual Studio, the instruction varies slightly, please see the CMake or Visual Studio documentation.

## Coding Guidelines

Match the project’s existing style. We use lower-case variables and function names separated by underscores. If you have docker on your system and a bash shell, you can apply clang-format to the code by calling `scripts/clang_format_docker.sh`.

All code should be pure ASCII, including the strings.

We do not use `assert` expressions in the code. We either always check a condition, or else we
only test it when logging is enabled (`simdutf_log_assert`).

It is sometimes beneficial to use logging or assertions to help debug code. Configure the build directory with the 
`SIMDUTF_LOGGING` variable set to `ON`.

```bash
cmake -B build -DSIMDUTF_LOGGING=ON
```

Then use `simdutf_log("message")` when logging. We also have a log-based assert: `simdutf_log_assert(cond, message)`. You should never use the simdutf library built with `SIMDUTF_LOGGING` set to ON in production, it is only for debugging.


Comments: Document complex logic clearly. It is fine to use multiple lines of text.

Dependencies: Avoid new external dependencies unless approved.

## Testing

All changes must have been tested.


Run our tests:
```bash
ctest --test-dir build
```

Add tests in the tests/directory for new features or fixes, covering edge cases and performance.
Use our macros when building tests:

```C++

TEST(roundtrip_base64_url_with_garbage) {
    ASSERT_TRUE(1==1);
    ASSERT_EQUAL("a", "b");
}
```

All optimizations should be based on benchmarks, please configure your projects with benchmarks.
```bash
cmake -B build -D SIMDUTF_BENCHMARKS=ON
cmake --build build
./build/benchmarks/benchmark 
./build/benchmarks/base64/benchmark_base64 
```

The scripts `./scripts/benchmark_print.py`and `./scripts/base64bench_print.py` can be used to produce good-looking before/after tables in MarkDown. Please consider using these scripts.

We recommend that you run our fuzzers as well, please see `fuzz/README.md`.



## Tips

### Running a single test

Our testing binaries allow you to run specific tests which can be convenient:

```
cmake --build build --target base64_tests  && ./build/tests/base64_tests -t roundtrip_base64_with_garbage
```

## Including fallback kernel

By default, we may disable the fallback kernel on some systems. You can force it on with `SIMDUTF_ALWAYS_INCLUDE_FALLBACK`:

```
cmake -B build  -DSIMDUTF_ALWAYS_INCLUDE_FALLBACK=ON
```


### C++20

We recommend running tests using C++20 since some of our functions and tests
are only available when C++20 is set.

```
cmake -B build20  -DSIMDUTF_CXX_STANDARD=20 -DSIMDUTF_ALWAYS_INCLUDE_FALLBACK=ON
```


### Adapting internal buffers

Some bugs are easier to track if internal buffers are reduced.

```
cmake -B buildfuzz20  -DSIMDUTF_CXX_STANDARD=20 -DFUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION=ON -DSIMDUTF_ALWAYS_INCLUDE_FALLBACK=ON
```

### Add sanitizers!

Try to combine these flags with the sanitizers and logging/asserts for best results.

```
cmake -B buildsani  -DSIMDUTF_CXX_STANDARD=20 -DFUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION=ON -DSIMDUTF_ALWAYS_INCLUDE_FALLBACK=ON -DSIMDUTF_SANITIZE=ON -DSIMDUTF_LOGGING=ON
```

## Documentation

Update documentation for any changes: Modify README.md or other files as needed. Provide clear examples for new features. Ensure accuracy and clarity.

## Licensing
Contributions are licensed under the project’s license. Ensure your work complies and does not infringe on third-party rights.

Thank you for contributing to simdutf! Your efforts help make this library faster and more robust. For questions, use GitHub Issues or discussions.

