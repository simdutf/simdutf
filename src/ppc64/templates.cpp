/*
    Template `convert_impl` implements generic conversion routine between
    different encodings. Procedure returns the number of written elements,
    or zero in the case of error.

    Parameters:
    * VectorizedConvert - vectorized procedure that returns structure having
      three fields: error_code (err), const Source* (input), Destination*
   (output)
    * ScalarConvert - scalar procedure that carries on conversion of tail
    * Source - type of input char (like char16_t, char)
    * Destination - type of input char
*/
template <typename VectorizedConvert, typename ScalarConvert, typename Source,
          typename Destination>
size_t convert_impl(VectorizedConvert vectorized_convert,
                    ScalarConvert scalar_convert, const Source *buf, size_t len,
                    Destination *output) {
  const auto vr = vectorized_convert(buf, len, output);
  const size_t consumed = vr.input - buf;
  const size_t written = vr.output - output;
  if (vr.err != simdutf::error_code::SUCCESS) {
    if (vr.err == simdutf::error_code::OTHER) {
      // Vectorized procedure detected an error, but does not know
      // exact position. The scalar procedure rescan the portion of
      // input and figure out where the error is located.
      return scalar_convert(vr.input, len - consumed, vr.output);
    }
    return 0;
  }

  if (consumed == len) {
    return written;
  }

  const auto ret = scalar_convert(vr.input, len - consumed, vr.output);
  if (ret == 0) {
    return 0;
  }

  return written + ret;
}

/*
    Template `convert_with_errors_impl` implements generic conversion routine
    between different encodings. Procedure returns a `result` instance ---
    please refer to its documentation for details.

    Parameters:
    * VectorizedConvert - vectorized procedure that returns structure having
      three fields: error_code (err), const Source* (input), Destination*
   (output)
    * ScalarConvert - scalar procedure that carries on conversion of tail
    * Source - type of input char (like char16_t, char)
    * Destination - type of input char
*/
template <typename VectorizedConvert, typename ScalarConvert, typename Source,
          typename Destination>
simdutf::result convert_with_errors_impl(VectorizedConvert vectorized_convert,
                                         ScalarConvert scalar_convert,
                                         const Source *buf, size_t len,
                                         Destination *output) {

  const auto vr = vectorized_convert(buf, len, output);
  const size_t consumed = vr.input - buf;
  const size_t written = vr.output - output;
  if (vr.err != simdutf::error_code::SUCCESS) {
    if (vr.err == simdutf::error_code::OTHER) {
      // Vectorized procedure detected an error, but does not know
      // exact position. The scalar procedure rescan the portion of
      // input and figure out where the error is located.
      auto sr = scalar_convert(vr.input, len - consumed, vr.output);
      sr.count += consumed;
      return sr;
    }
    return simdutf::result(vr.err, consumed);
  }

  if (consumed == len) {
    return simdutf::result(simdutf::error_code::SUCCESS, written);
  }

  simdutf::result sr = scalar_convert(vr.input, len - consumed, vr.output);
  if (sr.is_ok()) {
    sr.count += written;
  } else {
    sr.count += consumed;
  }

  return sr;
}
