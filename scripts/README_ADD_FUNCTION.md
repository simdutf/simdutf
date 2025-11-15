# Adding a New Function to simdutf

This guide explains how to use the `add_function.py` script to automate adding a new function to the simdutf library.

## Prerequisites
- Python 3 installed.
- The script assumes the repository structure is intact (e.g., `include/simdutf/implementation.h`, `src/implementation.cpp`, etc.).

## Steps
1. **Create a Signature File**: Create a text file (e.g., `new_function.sig`) containing the function signatures, wrapped in the appropriate feature macro block. You can include multiple functions in one block. Most functions in simdutf are marked `noexcept` and you should mark them `noexcept`. If they return a value, we typically marked the return value `simdutf_warn_unused`. Example:
   ```
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

3. **What the Script Does**:
   - Parses the signature file to extract the feature macro and all function signatures with their documentation.
   - Adds each function as a standalone function in the `simdutf` namespace in `include/simdutf/implementation.h`.
   - Adds each as a virtual function in the `implementation` class.
   - Adds implementations in `src/implementation.cpp` (detect, unsupported, and standalone).
   - Adds stub implementations (`// TODO: implement`) in all `src/*/implementation.cpp` files.

4. **Post-Script Steps**:
   - Implement the actual function logic in each `src/*/implementation.cpp` file.
   - Update any tests or documentation as needed.
   - Rebuild and test the library.

## Warnings
- This script modifies source files directly. Back up your repository before running it.
- The script uses simple text manipulation; complex signatures may require manual adjustments.
- Ensure the feature macro is correctly defined in the build system.

If you encounter issues, check the script's output and verify the file paths.