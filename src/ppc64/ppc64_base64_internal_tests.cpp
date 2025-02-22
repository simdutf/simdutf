// unit tests of internal implementation

static void unittest_encoding_translate_6bit_values() {
  const char *base64_std =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  const char *base64_url =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

  for (uint8_t value = 0; value < 64; value++) {
    const auto in = vector_u8::splat(value);
    const auto got = encoding_translate_6bit_values<false>(in);
    const auto want = vector_u8::splat(base64_std[value]);

    const auto eq = (got == want);
    if (not eq.all()) {
      printf("value = 0x%02x (%c)\n", value, value);
      printf("want  = ");
      want.dump();
      printf("got   = ");
      got.dump();
      abort();
    }
  }

  for (uint8_t value = 0; value < 64; value++) {
    const auto in = vector_u8::splat(value);
    const auto got = encoding_translate_6bit_values<true>(in);
    const auto want = vector_u8::splat(base64_url[value]);

    const auto eq = (got == want);
    if (not eq.all()) {
      printf("value = 0x%02x (%c)\n", value, value);
      printf("want  = ");
      want.dump();
      printf("got   = ");
      got.dump();
      abort();
    }
  }
}

static void unittest_encoding_expand_6bit_fields() {
  for (size_t position = 0; position < 10; position++) {
    for (uint64_t value = 0; value < 64; value++) {
      const uint64_t v = value << (6 * position);

      const vector_u8 input(
          uint8_t(v >> 0 * 8), uint8_t(v >> 1 * 8), uint8_t(v >> 2 * 8),
          uint8_t(v >> 3 * 8), uint8_t(v >> 4 * 8), uint8_t(v >> 5 * 8),
          uint8_t(v >> 6 * 8), uint8_t(v >> 7 * 8), 0, 0, 0, 0, 0, 0, 0, 0);
      const auto expanded = encoding_expand_6bit_fields(input);

      // test
      for (size_t i = 0; i < 10; i++) {
        uint8_t want = 0;
        if (i == position) {
          want = uint8_t(value);
        }

        const uint8_t got = expanded.value[i];
        if (got != want) {
          printf("value = 0x%02x\n", uint8_t(value));
          printf("pos   = %lu\n", position);
          printf("want  = %02x\n", want);
          printf("got   = %02x\n", got);
          printf("expanded = ");
          expanded.dump();
          abort();
        }
      }
    }
  }
}

static void unittest_implementation() {
  unittest_encoding_translate_6bit_values();
  unittest_encoding_expand_6bit_fields();
  puts("All OK");
}
