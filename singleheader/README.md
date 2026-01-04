

While in the singleheader directory under a linux or macOS system with an install toolchain, try:

```
c++ -o amalgamation_demo amalgamation_demo.cpp -std=c++17 && ./amalgamation_demo
```


### C Demo

You may also build a C executable.

```
c++ -c simdutf.cpp 
cc -c ./amalgamation_demo.c
c++ amalgamation_demo.o simdutf.o -o cdemo
./cdemo
```