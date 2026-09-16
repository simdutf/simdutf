# Adding a New Function to simdutf

This guide explains how to use the `add_function.py` script to automate adding a new function to the simdutf library.

## Prerequisites
- Python 3 installed.
- The script assumes the repository structure is intact (e.g., `include/simdutf/implementation.h`, `src/implementation.cpp`, etc.).

## Steps
1. **Create a Signature File**: Create a text file (e.g., `new_function.sig`) containing the function signatures, wrapped in the appropriate feature macro block. You can include multiple functions in one block. Most functions in simdutf are marked `noexcept` and you should mark them `noexcept`. If they return a value, we typically marked the return value `simdutf_warn_unused`. Example:
   ```C++
   #if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_UTF16
   /**
    * Documentation for first function.
    */
   simdutf_warn_unused size_t utf8_length_from_utf16le(
       const char16_t *buf, size_t len) noexcept;

   /**
    * Documentation for second function.
    */
   simdutf_warn_unused size_t utf8_length_from_utf16be(
       const char16_t *buf, size_t len) noexcept;
   #endif
   ```

2. **Run the Script**: Execute the script with the signature file as argument:
   ```shell
   python scripts/add_function.py new_function.sig
   ```
   Pass `--no-c-api` if the function should not be part of the C API.

3. **What the Script Does**:
   - Parses the signature file to extract the feature macro and all function signatures with their documentation.
   - Adds each function as a standalone function in the `simdutf` namespace in `include/simdutf/implementation.h`.
   - Adds each as a virtual function in the `implementation` class.
   - Adds implementations in `src/implementation.cpp` (detect, unsupported, and standalone).
   - Adds stub implementations (`// TODO: implement`) in all `src/*/implementation.cpp` files.
   - Adds a C wrapper to `include/simdutf_c.h` and `src/simdutf_c.cpp`, see below.

4. **Post-Script Steps**:
   - Implement the actual function logic in each `src/*/implementation.cpp` file.
   - Update any tests or documentation as needed.
   - Cover the C wrapper in `tests/straight_c_test.c` and `tests/nostdlibcxx_c_api_test.c`.
   - Rebuild and test the library.

## The C API

The C API is a thin forwarding layer: `simdutf_f(...)` calls `simdutf::f(...)`
and converts the types that C cannot spell. The script writes that layer for
you when every type of the signature has a C counterpart:

- plain types (`size_t`, `bool`, `char`, `char16_t`, `char32_t`, pointers to
  them, ...) are forwarded as they are;
- `result` and `full_result` are returned as `simdutf_result` and
  `simdutf_full_result`, through the `to_c_result` and `to_c_full_result`
  converters (a new struct is one more entry in `C_RESULT_TYPES`);
- `encoding_type`, `base64_options` and `last_chunk_handling_options` are cast
  to and from their `simdutf_` counterparts.

Anything else (references such as the `size_t &outlen` of
`base64_to_binary_safe`, `std::span` overloads, templates, `char8_t`, ...)
needs a wrapper written by hand: the script reports what it skipped and leaves
the two C API files alone. The same is true of anything that is not a plain
forwarding call, such as the `simdutf_base64_to_binary_safe` wrapper, which
has to bounce `outlen` through a local variable.

The wrappers land at the end of the `extern "C"` block of each file, so move
them next to the functions they belong with, and check that the generated
documentation reads well as a C comment. The C API sits behind a single `#if`
requiring every feature, so no feature macro has to be threaded through.

Note that the whole C API is regenerated into `singleheader/simdutf_c.h` by
`singleheader/amalgamate.py`, so nothing else is needed for the single-header
distribution.

## Warnings
- This script modifies source files directly. Back up your repository before running it.
- The script uses simple text manipulation; complex signatures may require manual adjustments. Always review the diff.
- Ensure the feature macro is correctly defined in the build system.

If you encounter issues, check the script's output and verify the file paths.