

While in the singleheader directory under a linux or macOS system with an install toolchain, try:

```
c++ -o amalgamation_demo amalgamation_demo.cpp -std=c++17 && ./amalgamation_demo
```


### C Demo

You may also build a C executable without a dependency on the C++ standard library.

```
c++ -c simdutf.cpp  -nostdlib++ -fno-rtti -fno-exceptions
cc amalgamation_demo.c simdutf.o -o cdemo
./cdemo
```