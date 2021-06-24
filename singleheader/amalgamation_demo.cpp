#include <iostream>
#include "simdutf.h"
#include "simdutf.cpp"

int main(int argc, char *argv[]) {
   const char * ascii = "1234";
   bool validutf8 = simdutf::validate_utf8(ascii, 4);
   if(validutf8) {
       std::cout << "valid UTF-8" << std::endl;
   } else {
       std::cout << "invalid UTF-8" << std::endl;
       return EXIT_FAILURE;
   }
   char16_t utf16_output[4];
   size_t utf16words = simdutf::convert_utf8_to_utf16(ascii, 4, utf16_output);
   std::cout << "wrote " << utf16words << " UTF-16 words." << std::endl;
   return EXIT_SUCCESS;
}