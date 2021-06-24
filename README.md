[![Alpine Linux](https://github.com/lemire/simdutf/actions/workflows/alpine.yml/badge.svg)](https://github.com/lemire/simdutf/actions/workflows/alpine.yml)
[![MSYS2-CI](https://github.com/lemire/simdutf/actions/workflows/msys2.yml/badge.svg)](https://github.com/lemire/simdutf/actions/workflows/msys2.yml)
[![MSYS2-CLANG-CI](https://github.com/lemire/simdutf/actions/workflows/msys2-clang.yml/badge.svg)](https://github.com/lemire/simdutf/actions/workflows/msys2-clang.yml)
[![Ubuntu 20.04 CI (GCC 9)](https://github.com/lemire/simdutf/actions/workflows/ubuntu20sani.yml/badge.svg)](https://github.com/lemire/simdutf/actions/workflows/ubuntu20sani.yml)
[![VS16-ARM-CI](https://github.com/lemire/simdutf/actions/workflows/vs16-arm-ci.yml/badge.svg)](https://github.com/lemire/simdutf/actions/workflows/vs16-arm-ci.yml)
[![VS16-CI](https://github.com/lemire/simdutf/actions/workflows/vs16-ci.yml/badge.svg)](https://github.com/lemire/simdutf/actions/workflows/vs16-ci.yml)

simdutf: insanely fast Unicode validation and transcoding
===============================================

This library provide fast Unicode functions such as

- UTF-8 and UTF-16 validation
- UTF-8 to UTF-16 transcoding, with or without validation.
- UTF-16 to UTF-8 transcoding, with or without validation.
- UTF-8 and UTF-16 character counting

The functions are accelerated using SIMD instructions
(e.g., ARM NEON, SSE, AVX, etc.).

Requirements
-------

- C++11 compatible compiler. We support LLVM clang, GCC, Visual Studio.
- Recent CMake (at least 3.15)

Usage
-------

```
cmake -B build
cmake --build build
cd build
ctest .
```

To run benchmarks, execute the `benchmark` command. You can get help on its
usage by first building it and then calling it with the `--help` flag.
E.g., under Linux you may do the following:

```
cmake -B build
cmake --build build
./build/benchmarks/benchmark --help
```

Testing data
------------

We recommend the following data repository.

https://github.com/lemire/unicode_lipsum

License
-------

This code is made available under the [Apache License 2.0](https://www.apache.org/licenses/LICENSE-2.0.html) as well as the MIT license.

We include a few competitive solutions under the benchmarks/competition directory. They are provided for
research purposes only.
