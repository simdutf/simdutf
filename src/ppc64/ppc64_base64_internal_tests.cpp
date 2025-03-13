namespace base64tests {
const char *base64_std =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
const char *base64_url =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
const char *accepted_whitespaces = "\x20\x09\x0a\x0c\x0d";
} // namespace base64tests

[[maybe_unused]] static void
base64_encoding_translate_6bit_values(const simdutf::implementation &) {
  using simdutf::ppc64::encoding_translate_6bit_values;
  using simdutf::ppc64::vector_u8;

  for (uint8_t value = 0; value < 64; value++) {
    const auto in = vector_u8::splat(value);
    const auto got = encoding_translate_6bit_values<false>(in);
    const auto want = vector_u8::splat(base64tests::base64_std[value]);

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
    const auto want = vector_u8::splat(base64tests::base64_url[value]);

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

[[maybe_unused]] static void
base64_encoding_expand_6bit_fields(const simdutf::implementation &) {
  using simdutf::ppc64::encoding_expand_6bit_fields;
  using simdutf::ppc64::vector_u8;

  for (size_t position = 0; position < 5; position++) {
    for (uint32_t value = 0; value < 0xffffff; value++) {
      const uint8_t a = uint8_t(value >> (3 * 6)) & 0x3f;
      const uint8_t b = uint8_t(value >> (2 * 6)) & 0x3f;
      const uint8_t c = uint8_t(value >> (1 * 6)) & 0x3f;
      const uint8_t d = uint8_t(value >> (0 * 6)) & 0x3f;

      auto input = vector_u8::zero();
      input.value[position * 3 + 0] = uint8_t(value >> (2 * 8));
      input.value[position * 3 + 1] = uint8_t(value >> (1 * 8));
      input.value[position * 3 + 2] = uint8_t(value >> (0 * 8));

      const auto expanded = encoding_expand_6bit_fields(input);

      // test
      const size_t a_idx = position * 4 + 0;
      const size_t b_idx = position * 4 + 1;
      const size_t c_idx = position * 4 + 2;
      const size_t d_idx = position * 4 + 3;

      for (size_t i = 0; i < 5; i++) {
        uint8_t want = 0;
        if (i == a_idx) {
          want = a;
        } else if (i == b_idx) {
          want = b;
        } else if (i == c_idx) {
          want = c;
        } else if (i == d_idx) {
          want = d;
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

[[maybe_unused]] static void
base64_decoding_valid(const simdutf::implementation &) {
  using simdutf::ppc64::block64;
  using simdutf::ppc64::vector_u8;
  using simdutf::ppc64::with_base64_std;
  using simdutf::ppc64::with_base64_url;
  using simdutf::ppc64::with_ignore_errors;

  for (uint8_t i = 0; i < 64; i++) {
    auto ascii = vector_u8::splat(base64tests::base64_std[i]);
    uint16_t error = 0;
    const auto mask =
        block64::to_base64_mask<with_base64_std, with_ignore_errors>(ascii,
                                                                     error);

    const auto err =
        (mask != 0) || (error != 0) || (ascii != vector_u8::splat(i)).any();

    if (err) {
      printf("mask  = %02x\n", mask);
      printf("error = %02x\n", error);
      printf("want  = 0x%02x\n", i);
      printf("got   = ");
      ascii.dump();
      abort();
    }
  }

  for (uint8_t i = 0; i < 64; i++) {
    auto ascii = vector_u8::splat(base64tests::base64_url[i]);
    uint16_t error = 0;
    const auto mask =
        block64::to_base64_mask<with_base64_url, with_ignore_errors>(ascii,
                                                                     error);

    const auto err =
        (mask != 0) || (error != 0) || (ascii != vector_u8::splat(i)).any();

    if (err) {
      printf("mask  = %02x\n", mask);
      printf("error = %02x\n", error);
      printf("want  = 0x%02x\n", i);
      printf("got   = ");
      ascii.dump();
      abort();
    }
  }
}

template <bool base64_url>
static void unittest_decoding_invalid_ignore_errors(const char *base64) {
  using simdutf::ppc64::block64;
  using simdutf::ppc64::vector_u8;
  using simdutf::ppc64::with_ignore_errors;

  const char *accepted_whitespaces = " \t\n\r\v";

  constexpr uint8_t invalid = 0xff;
  constexpr uint8_t whitespace = 0x80;

  uint8_t map[256];
  for (size_t i = 0; i < 256; i++) {
    map[i] = invalid;
  }

  for (uint8_t i = 0; i < 64; i++) {
    map[uint8_t(base64[i])] = i;
  }

  for (size_t i = 0; i < 5; i++) {
    const uint8_t idx = uint8_t(accepted_whitespaces[i]);
    map[idx] = whitespace;
  }

  for (size_t i = 0; i < 256; i++) {
    const auto b = uint8_t(i);
    auto ascii = vector_u8::splat(b);
    uint16_t error = 0;
    const auto mask =
        block64::to_base64_mask<base64_url, with_ignore_errors>(ascii, error);

    if (map[i] == invalid or map[i] == whitespace) {
      if (mask != 0xffff or error != 0x0000) {
        printf("value = %u (0x%02x)\n", b, b);
        printf("mask  = %04x\n", mask);
        printf("error = %04x\n", error);
        ascii.dump();
        printf("%s:%d\n", __FILE__, __LINE__);
        exit(1);
      }
    } else {
      if (mask != 0x0000 or error != 0x0000) {
        printf("value = %u (0x%02x)\n", b, b);
        printf("mask  = %04x\n", mask);
        printf("error = %04x\n", error);
        ascii.dump();
        printf("%s:%d\n", __FILE__, __LINE__);
        exit(1);
      }
    }
  }
}

template <bool base64_url>
static void unittest_decoding_invalid_strict_errors(const char *base64) {
  using simdutf::ppc64::block64;
  using simdutf::ppc64::vector_u8;
  using simdutf::ppc64::with_strict_checking;

  constexpr uint8_t invalid = 0xff;
  constexpr uint8_t whitespace = 0x80;

  uint8_t map[256];
  for (size_t i = 0; i < 256; i++) {
    map[i] = invalid;
  }

  for (uint8_t i = 0; i < 64; i++) {
    map[uint8_t(base64[i])] = i;
  }

  for (size_t i = 0; i < 5; i++) {
    const uint8_t idx = uint8_t(base64tests::accepted_whitespaces[i]);
    map[idx] = whitespace;
  }

  for (size_t i = 0; i < 256; i++) {
    const uint8_t b = uint8_t(i);
    auto ascii = vector_u8::splat(b);
    uint16_t error = 0;
    const auto mask =
        block64::to_base64_mask<base64_url, with_strict_checking>(ascii, error);

    if (map[i] == invalid) {
      if (mask != 0xffff or error != 0xffff) {
        printf("value = %u (0x%02x)\n", b, b);
        printf("mask  = %04x\n", mask);
        printf("error = %04x\n", error);
        ascii.dump();
        printf("%s:%d\n", __FILE__, __LINE__);
        exit(1);
      }
    } else if (map[i] == whitespace) {
      if (mask != 0xffff or error != 0x0000) {
        printf("value = %lu (0x%02lx)\n", i, i);
        printf("mask  = %04x\n", mask);
        printf("error = %04x\n", error);
        ascii.dump();
        printf("%s:%d\n", __FILE__, __LINE__);
        exit(1);
      }
    } else {
      if (mask != 0x0000 or error != 0x0000) {
        printf("value = %lu (0x%02lx)\n", i, i);
        printf("mask  = %04x\n", mask);
        printf("error = %04x\n", error);
        ascii.dump();
        printf("%s:%d\n", __FILE__, __LINE__);
        exit(1);
      }
    }
  }
}

[[maybe_unused]] static void
base64_decoding_invalid_ignore_errors(const simdutf::implementation &) {
  using simdutf::ppc64::with_base64_std;

  unittest_decoding_invalid_ignore_errors<with_base64_std>(
      base64tests::base64_std);
}

[[maybe_unused]] static void
base64url_decoding_invalid_ignore_errors(const simdutf::implementation &) {
  using simdutf::ppc64::with_base64_url;

  unittest_decoding_invalid_ignore_errors<with_base64_url>(
      base64tests::base64_url);
}

[[maybe_unused]] static void
base64_decoding_invalid_strict_errors(const simdutf::implementation &) {
  using simdutf::ppc64::with_base64_std;

  unittest_decoding_invalid_strict_errors<with_base64_std>(
      base64tests::base64_std);
}

[[maybe_unused]] static void
base64url_decoding_invalid_strict_errors(const simdutf::implementation &) {
  using simdutf::ppc64::with_base64_url;

  unittest_decoding_invalid_strict_errors<with_base64_url>(
      base64tests::base64_url);
}

[[maybe_unused]] static void
base64_decoding_pack(const simdutf::implementation &) {
  using simdutf::ppc64::decoding_pack;
  using simdutf::ppc64::vector_u8;

  for (size_t position = 0; position < 4; position++) {
    for (uint32_t value = 0; value < 0xffffff; value++) {
      const uint8_t a = uint8_t(value >> (0 * 6)) & 0x3f;
      const uint8_t b = uint8_t(value >> (1 * 6)) & 0x3f;
      const uint8_t c = uint8_t(value >> (2 * 6)) & 0x3f;
      const uint8_t d = uint8_t(value >> (3 * 6)) & 0x3f;

      auto input = vector_u8::zero();
      input.value[position * 4 + 0] = a;
      input.value[position * 4 + 1] = b;
      input.value[position * 4 + 2] = c;
      input.value[position * 4 + 3] = d;

      const uint8_t b0 = uint8_t((a << 2) | (b >> 4));
      const uint8_t b1 = uint8_t((b << 4) | (c >> 2));
      const uint8_t b2 = uint8_t(d | (c << 6));

      const auto packed = decoding_pack(input);

      // test
      const size_t b0_idx = position * 3 + 0;
      const size_t b1_idx = position * 3 + 1;
      const size_t b2_idx = position * 3 + 2;

      for (size_t i = 0; i < 3; i++) {
        uint8_t want = 0;
        if (i == b0_idx) {
          want = b0;
        } else if (i == b1_idx) {
          want = b1;
        } else if (i == b2_idx) {
          want = b2;
        }

        const uint8_t got = packed.value[i];
        if (got != want) {
          printf("pos   = %lu\n", position);
          printf("want  = %02x\n", want);
          printf("got   = %02x\n", got);
          printf("expanded = ");
          packed.dump();
          printf("file %s:%d, function %s\n", __FILE__, __LINE__, __func__);
          abort();
        }
      }
    }
  }
}

[[maybe_unused]] static int
scalar_compress(const simdutf::ppc64::vector_u8 data, uint16_t nmask,
                char *output) {
  char tmp[16];
  data.store(tmp);

  int j = 0;
  for (int i = 0; i < 16; i++) {
    if ((nmask & 0x1) == 0) {
      output[j++] = tmp[i];
    }

    nmask >>= 1;
  }

  return j;
}

[[maybe_unused]] static void base64_compress(const simdutf::implementation &) {
  using simdutf::ppc64::compress;
  using simdutf::ppc64::vector_u8;

  char want[16];
  char got[16];

  const auto data = vector_u8('A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
                              'K', 'L', 'M', 'N', 'O', 'P');
  for (uint32_t mask = 0; mask <= 0xffff; mask++) {
    memset(want, 0, 16);
    memset(got, 0, 16);
    const uint16_t nmask = uint16_t(~mask);

    const int count = scalar_compress(data, nmask, want);
    compress(data, nmask, got);

    if (memcmp(want, got, count) != 0) {
      printf("want = ");
      for (int i = 0; i < count; i++) {
        putchar(want[i]);
      }
      putchar('\n');

      printf("got  = ");
      for (int i = 0; i < count; i++) {
        putchar(got[i]);
      }
      putchar('\n');

      exit(1);
    }
  }
}
