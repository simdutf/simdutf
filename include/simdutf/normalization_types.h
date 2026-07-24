#ifndef SIMDUTF_NORMALIZATION_TYPES_H
#define SIMDUTF_NORMALIZATION_TYPES_H

namespace simdutf {

enum class ComposedForm { NFC, NFKC };
enum class DecomposedForm { NFD, NFKD };

constexpr DecomposedForm to_decomposed_form(ComposedForm form) {
  switch (form) {
  case ComposedForm::NFC:
    return DecomposedForm::NFD;
  case ComposedForm::NFKC:
    return DecomposedForm::NFKD;
  }
}

} // namespace simdutf

#endif // SIMDUTF_NORMALIZATION_TYPES_H
