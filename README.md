simdutf: insanely fast Unicode/ 
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

- C++11 compatible compiler.
- Recent CMake (at least 3.13)

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
