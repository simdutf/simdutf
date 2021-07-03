#include <iostream>
#include "simdutf.h"
#include "simdutf.cpp"

int main(int argc, char *argv[]) {
   // 4 == strlen(ascii)
   bool validutf8 = simdutf::validate_utf8(ascii, 4);
   if(validutf8) {
       std::cout << "valid UTF-8" << std::endl;
   } else {
       std::cout << "invalid UTF-8" << std::endl;
       return EXIT_FAILURE;
   }
   // We need a buffer of size simdutf::utf16length_from_utf8(ascii, 4).
   char16_t utf16_output[4];
   // convert to UTF-16LE
   size_t utf16words = simdutf::convert_utf8_to_utf16(ascii, 4, utf16_output);
   std::cout << "wrote " << utf16words << " UTF-16 words." << std::endl;
   // It wrote utf16words * sizeof(char16_t) bytes.
   //
   // convert it back:
   char buffer[4];
   size_t utf8words = simdutf::convert_utf16_to_utf8(utf16_output, utf16words, buffer);
   std::cout << "wrote " << utf8words << " UTF-8 words." << std::endl;
   return EXIT_SUCCESS;
}