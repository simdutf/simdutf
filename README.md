
simdutf 
===============================================


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

License
-------

This code is made available under the [Apache License 2.0](https://www.apache.org/licenses/LICENSE-2.0.html).

For compilers that do not support [C++17](https://en.wikipedia.org/wiki/C%2B%2B17), we bundle the string-view library which is published under the Boost license (http://www.boost.org/LICENSE_1_0.txt). Like the Apache license, the Boost license is a permissive license allowing commercial redistribution.
