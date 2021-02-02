#ifndef SIMDUTF_H
#define SIMDUTF_H

/**
 * @mainpage
 *
 * Check the [README.md](https://github.com/lemire/simdutf/blob/master/README.md#simdutf--parsing-gigabytes-of-json-per-second).
 *
 * Sample code. See https://github.com/simdutf/simdutf/blob/master/doc/basics.md for more examples.

    #include "simdutf.h"

    int main(void) {
      // load from `twitter.json` file:
      simdutf::dom::parser parser;
      simdutf::dom::element tweets = parser.load("twitter.json");
      std::cout << tweets["search_metadata"]["count"] << " results." << std::endl;

      // Parse and iterate through an array of objects
      auto abstract_json = R"( [
        {  "12345" : {"a":12.34, "b":56.78, "c": 9998877}   },
        {  "12545" : {"a":11.44, "b":12.78, "c": 11111111}  }
        ] )"_padded;

      for (simdutf::dom::object obj : parser.parse(abstract_json)) {
        for(const auto key_value : obj) {
          cout << "key: " << key_value.key << " : ";
          simdutf::dom::object innerobj = key_value.value;
          cout << "a: " << double(innerobj["a"]) << ", ";
          cout << "b: " << double(innerobj["b"]) << ", ";
          cout << "c: " << int64_t(innerobj["c"]) << endl;
        }
      }
    }
 */

#include "simdutf/compiler_check.h"
#include "simdutf/common_defs.h"

SIMDUTF_PUSH_DISABLE_WARNINGS
SIMDUTF_DISABLE_UNDESIRED_WARNINGS

// Public API
#include "simdutf/simdutf_version.h"
#include "simdutf/error.h"
#include "simdutf/minify.h"
#include "simdutf/padded_string.h"
#include "simdutf/implementation.h"
#include "simdutf/dom/array.h"
#include "simdutf/dom/document_stream.h"
#include "simdutf/dom/document.h"
#include "simdutf/dom/element.h"
#include "simdutf/dom/object.h"
#include "simdutf/dom/parser.h"
#include "simdutf/dom/serialization.h"

// Deprecated API
#include "simdutf/dom/jsonparser.h"
#include "simdutf/dom/parsedjson.h"
#include "simdutf/dom/parsedjson_iterator.h"

// Inline functions
#include "simdutf/dom/array-inl.h"
#include "simdutf/dom/document_stream-inl.h"
#include "simdutf/dom/document-inl.h"
#include "simdutf/dom/element-inl.h"
#include "simdutf/error-inl.h"
#include "simdutf/dom/object-inl.h"
#include "simdutf/padded_string-inl.h"
#include "simdutf/dom/parsedjson_iterator-inl.h"
#include "simdutf/dom/parser-inl.h"
#include "simdutf/internal/tape_ref-inl.h"
#include "simdutf/dom/serialization-inl.h"

// Implementation-internal files (must be included before the implementations themselves, to keep
// amalgamation working--otherwise, the first time a file is included, it might be put inside the
// #ifdef SIMDUTF_IMPLEMENTATION_ARM64/FALLBACK/etc., which means the other implementations can't
// compile unless that implementation is turned on).
#include "simdutf/internal/isadetection.h"
#include "simdutf/internal/jsoncharutils_tables.h"
#include "simdutf/internal/numberparsing_tables.h"
#include "simdutf/internal/simdprune_tables.h"

// Implementations
#include "simdutf/arm64.h"
#include "simdutf/haswell.h"
#include "simdutf/westmere.h"
#include "simdutf/ppc64.h"
#include "simdutf/fallback.h"
#include "simdutf/builtin.h"

SIMDUTF_POP_DISABLE_WARNINGS

#endif // SIMDUTF_H
