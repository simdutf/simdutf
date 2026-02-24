
namespace simdutf {
std::string to_string(encoding_type bom) {
  switch (bom) {
  case UTF16_LE:
    return "UTF16 little-endian";
  case UTF16_BE:
    return "UTF16 big-endian";
  case UTF32_LE:
    return "UTF32 little-endian";
  case UTF32_BE:
    return "UTF32 big-endian";
  case UTF8:
    return "UTF8";
  case unspecified:
    return "unknown";
  default:
    return "error";
  }
}

namespace BOM {
// Note that BOM for UTF8 is discouraged.
size_t bom_byte_size(encoding_type bom) {
  switch (bom) {
  case UTF16_LE:
    return 2;
  case UTF16_BE:
    return 2;
  case UTF32_LE:
    return 4;
  case UTF32_BE:
    return 4;
  case UTF8:
    return 3;
  case unspecified:
    return 0;
  default:
    return 0;
  }
}

} // namespace BOM
} // namespace simdutf
