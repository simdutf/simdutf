

While in the singleheader directory under a linux or macOS system with an install toolchain, try:

```
c++ -o amalgamation_demo amalgamation_demo.cpp -std=c++17 && ./amalgamation_demo
```


### C Demo

You can compile both the simdutf library and the C program using a C++ compiler.

```
c++ -c simdutf.cpp -std=c++17
cc -c amalgamation_demo.c
c++  amalgamation_demo.o simdutf.o -o cdemo
 ./cdemo
```

You may also build a C executable without a dependency on the C++ standard library.

```
c++ -c simdutf.cpp  -nostdlib++ -fno-rtti -fno-exceptions -DSIMDUTF_NO_LIBCXX=1 -std=c++17
cc amalgamation_demo.c simdutf.o -o cdemo
./cdemo
```