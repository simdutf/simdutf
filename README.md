
simdutf 
===============================================

This library provide fast Unicode functions such as

- UTF-8 and UTF-16 validation
- UTF-8 to UTF-16 transcoding, with or without validation.
- UTF-16 to UTF-8 transcoding, with or without validation.

Functions are accelerated using SIMD instructions (e.g., ARM NEON, SSE, AVX, etc.).

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

Testing data
------------

https://github.com/lemire/unicode_lipsum

License
-------

This code is made available under the [Apache License 2.0](https://www.apache.org/licenses/LICENSE-2.0.html) as well as the MIT license.
