# LoongArch64
You can now build and run the LoongArch64 code as needed:
GCC >= 14.1
Binutils >= 2.41

Note that the compiler may be named `loongarch64-linux-gnu-g++`, without the `unknown` part.
Please adjust to your system.

```
$ mkdir build && cd build
$ cmake -DCMAKE_CXX_COMPILER="loongarch64-unknown-linux-gnu-g++" \
  -DCMAKE_C_COMPILER="loongarch64-unknown-linux-gnu-gcc" \
  -DCMAKE_CROSSCOMPILING=True \
  -DCMAKE_CXX_FLAGS="-mlsx -mlasx " \
  -DCMAKE_BUILD_TYPE=Release ../
$ make
```
or
```
$ mkdir build && cd build
$ cmake -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-dev/loongarch64.cmake ../
$ make
```

Running tests with qemu
```
$ export QEMU_LD_PREFIX="/path_to_sysroot/"
$ export QEMU_CPU="la464"
$ make test
or
$ qemu-loongarch64 tests/base64_tests
```
