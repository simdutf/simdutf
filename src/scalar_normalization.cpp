
namespace simdutf {

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_NFD
const char *find_first_stable_utf8_nfd(const char *input,
                                       size_t length) noexcept {
  return scalar::utf8_to_decomposed::find_first_stable<DecomposedForm::NFD>(
      input, length);
}
const char *find_last_stable_utf8_nfd(const char *input,
                                      size_t length) noexcept {
  return scalar::utf8_to_decomposed::find_last_stable<DecomposedForm::NFD>(
      input, length);
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_NFD

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_NFKD
const char *find_first_stable_utf8_nfkd(const char *input,
                                        size_t length) noexcept {
  return scalar::utf8_to_decomposed::find_first_stable<DecomposedForm::NFKD>(
      input, length);
}
const char *find_last_stable_utf8_nfkd(const char *input,
                                       size_t length) noexcept {
  return scalar::utf8_to_decomposed::find_last_stable<DecomposedForm::NFKD>(
      input, length);
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_NFKD

#if SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_NFD
const char16_t *find_first_stable_utf16le_nfd(const char16_t *input,
                                              size_t length) noexcept {
  return scalar::utf16_to_decomposed::find_first_stable<endianness::LITTLE,
                                                        DecomposedForm::NFD>(
      input, length);
}
const char16_t *find_last_stable_utf16le_nfd(const char16_t *input,
                                             size_t length) noexcept {
  return scalar::utf16_to_decomposed::find_last_stable<endianness::LITTLE,
                                                       DecomposedForm::NFD>(
      input, length);
}
const char16_t *find_first_stable_utf16be_nfd(const char16_t *input,
                                              size_t length) noexcept {
  return scalar::utf16_to_decomposed::find_first_stable<endianness::BIG,
                                                        DecomposedForm::NFD>(
      input, length);
}
const char16_t *find_last_stable_utf16be_nfd(const char16_t *input,
                                             size_t length) noexcept {
  return scalar::utf16_to_decomposed::find_last_stable<endianness::BIG,
                                                       DecomposedForm::NFD>(
      input, length);
}
#endif // SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_NFD

#if SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_NFKD
const char16_t *find_first_stable_utf16le_nfkd(const char16_t *input,
                                               size_t length) noexcept {
  return scalar::utf16_to_decomposed::find_first_stable<endianness::LITTLE,
                                                        DecomposedForm::NFKD>(
      input, length);
}
const char16_t *find_last_stable_utf16le_nfkd(const char16_t *input,
                                              size_t length) noexcept {
  return scalar::utf16_to_decomposed::find_last_stable<endianness::LITTLE,
                                                       DecomposedForm::NFKD>(
      input, length);
}
const char16_t *find_first_stable_utf16be_nfkd(const char16_t *input,
                                               size_t length) noexcept {
  return scalar::utf16_to_decomposed::find_first_stable<endianness::BIG,
                                                        DecomposedForm::NFKD>(
      input, length);
}
const char16_t *find_last_stable_utf16be_nfkd(const char16_t *input,
                                              size_t length) noexcept {
  return scalar::utf16_to_decomposed::find_last_stable<endianness::BIG,
                                                       DecomposedForm::NFKD>(
      input, length);
}
#endif // SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_NFKD

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_NFC
const char *find_first_stable_utf8_nfc(const char *input,
                                       size_t length) noexcept {
  return scalar::normalization::find_first_stable<
      scalar::normalization::utf8_normalization_traits, ComposedForm::NFC>(
      input, length);
}
const char *find_last_stable_utf8_nfc(const char *input,
                                      size_t length) noexcept {
  return scalar::normalization::find_last_stable<
      scalar::normalization::utf8_normalization_traits, ComposedForm::NFC>(
      input, length);
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_NFC

#if SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_NFKC
const char *find_first_stable_utf8_nfkc(const char *input,
                                        size_t length) noexcept {
  return scalar::normalization::find_first_stable<
      scalar::normalization::utf8_normalization_traits, ComposedForm::NFKC>(
      input, length);
}
const char *find_last_stable_utf8_nfkc(const char *input,
                                       size_t length) noexcept {
  return scalar::normalization::find_last_stable<
      scalar::normalization::utf8_normalization_traits, ComposedForm::NFKC>(
      input, length);
}
#endif // SIMDUTF_FEATURE_UTF8 && SIMDUTF_FEATURE_NFKC

#if SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_NFC
const char16_t *find_first_stable_utf16le_nfc(const char16_t *input,
                                              size_t length) noexcept {
  return scalar::normalization::find_first_stable<
      scalar::normalization::utf16_normalization_traits<endianness::LITTLE>,
      ComposedForm::NFC>(input, length);
}
const char16_t *find_last_stable_utf16le_nfc(const char16_t *input,
                                             size_t length) noexcept {
  return scalar::normalization::find_last_stable<
      scalar::normalization::utf16_normalization_traits<endianness::LITTLE>,
      ComposedForm::NFC>(input, length);
}
const char16_t *find_first_stable_utf16be_nfc(const char16_t *input,
                                              size_t length) noexcept {
  return scalar::normalization::find_first_stable<
      scalar::normalization::utf16_normalization_traits<endianness::BIG>,
      ComposedForm::NFC>(input, length);
}
const char16_t *find_last_stable_utf16be_nfc(const char16_t *input,
                                             size_t length) noexcept {
  return scalar::normalization::find_last_stable<
      scalar::normalization::utf16_normalization_traits<endianness::BIG>,
      ComposedForm::NFC>(input, length);
}
#endif // SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_NFC

#if SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_NFKC
const char16_t *find_first_stable_utf16le_nfkc(const char16_t *input,
                                               size_t length) noexcept {
  return scalar::normalization::find_first_stable<
      scalar::normalization::utf16_normalization_traits<endianness::LITTLE>,
      ComposedForm::NFKC>(input, length);
}
const char16_t *find_last_stable_utf16le_nfkc(const char16_t *input,
                                              size_t length) noexcept {
  return scalar::normalization::find_last_stable<
      scalar::normalization::utf16_normalization_traits<endianness::LITTLE>,
      ComposedForm::NFKC>(input, length);
}
const char16_t *find_first_stable_utf16be_nfkc(const char16_t *input,
                                               size_t length) noexcept {
  return scalar::normalization::find_first_stable<
      scalar::normalization::utf16_normalization_traits<endianness::BIG>,
      ComposedForm::NFKC>(input, length);
}
const char16_t *find_last_stable_utf16be_nfkc(const char16_t *input,
                                              size_t length) noexcept {
  return scalar::normalization::find_last_stable<
      scalar::normalization::utf16_normalization_traits<endianness::BIG>,
      ComposedForm::NFKC>(input, length);
}
#endif // SIMDUTF_FEATURE_UTF16 && SIMDUTF_FEATURE_NFKC

} // namespace simdutf
