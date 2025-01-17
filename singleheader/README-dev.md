# Selective amalgamation

This short text is intended for the `simdutf` developers.

## How to enable/disable given piece of code

We use the following macros:

* `SIMDUTF_FEATURE_BASE64`
* `SIMDUTF_FEATURE_UTF8`
* `SIMDUTF_FEATURE_UTF16`
* `SIMDUTF_FEATURE_UTF32`
* `SIMDUTF_FEATURE_ASCII`
* `SIMDUTF_FEATURE_LATIN1`
* `SIMDUTF_FEATURE_DETECT_ENCODING`

The macros have by default value 1. Only during amalgamation
their values can be altered to 0.

Technically, there are two constraints on `SIMDUTF_FEATURE_XYZ`
flags:

* They must not be nested. Use logical operators for something
  more complex, like: `...UTF8 && (...UTF16 || ...UTF32)`.
* The corresponding `endif` must contain a repeated condition,
  of course commented out, like `#endif // SIMDUTF_FEATURE_BASE64`.

When we select given set of features, the API of library
is reduced only to these features. It is the responsibility
of users to detect whether they header having the expected
set of features.


## Testing

Making sure that selective amalgamation works as expected
requires only a little work.

We assume that at least `c++` command is available.

1. First of all, we need to execute `./test-features.py`.
   It requires Python3.4, as we extensively use `pathlib`.
2. The script will detect what additional crosscompilers
   are available on the machine, and will generate
   `Makefile` using this information.
3. Finally run `make Makefile`, preferably with `-j` argument.

Our `Makefile` generates amalgamated library with different variants
of features; the variants are predefined in `test-fetures.py`. Each
feature set is compiled with all detected C++ compilers.

A sample test session looks like that:

```
$ ./test-features.py
Found the following crosscompilers in $PATH:
aarch64         : aarch64-linux-gnu-g++-14
powerpc64       : powerpc64-linux-gnu-g++
loongarch64     : loongarch64-linux-gnu-g++-14
loongarch64lasx : loongarch64-linux-gnu-g++-14
riscv64         : riscv64-linux-gnu-g++-14

$ make help
make all             --- build all targets
make clear           --- remove all generated files
make help            --- show this help

make default         --- build using c++
make aarch64         --- build using aarch64-linux-gnu-g++-14
make powerpc64       --- build using powerpc64-linux-gnu-g++
make loongarch64     --- build using loongarch64-linux-gnu-g++-14
make loongarch64lasx --- build using loongarch64-linux-gnu-g++-14
make riscv64         --- build using riscv64-linux-gnu-g++-14

$ make -j<some-sane-number>
...
```

Please do not commit `Makefile`, it's tailored for the
machine where `test-features.py` was run.

