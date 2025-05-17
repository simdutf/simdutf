#include <chrono>
#include <cstddef>
#include <cstdint>
#include <array>
#include <format>
#include <iomanip>
#include <iostream>
#include <variant>

#include "helpers/common.h"
#include "simdutf.h"

constexpr std::array options = {
    simdutf::base64_default,
    simdutf::base64_url,
    simdutf::base64_default_no_padding,
    simdutf::base64_url_with_padding,
};

constexpr std::array last_chunk = {
    simdutf::last_chunk_handling_options::loose,
    simdutf::last_chunk_handling_options::strict,
    simdutf::last_chunk_handling_options::stop_before_partial};

struct decode_result {
  std::vector<char> binary;
  simdutf::result result;
};

template <typename T> std::string get_code(T c) {
  static_assert(std::is_same_v<T, char> || std::is_same_v<T, char16_t>,
                "T must be char or char16_t");

  using output_type =
      std::conditional_t<std::is_same_v<T, char>, uint8_t, uint16_t>;
  auto value = static_cast<output_type>(c);
  if (c == '\n') {
    return "'\\n'";
  } else if (c == '\r') {
    return "'\\r'";
  } else if (c == '\t') {
    return "'\\t'";
  } else if (c == '\f') {
    return "'\\f'";
  } else if (c == '\\') {
    return "'\\\\'";
  } else if (value >= 32 && value <= 126) { // Printable ASCII range
    return "'" + std::string(1, static_cast<char>(value)) + "'";
  } else {
    std::ostringstream oss;
    oss << "'" << (std::is_same_v<T, char> ? "\\x" : "\\u") << std::hex
        << std::setw(std::is_same_v<T, char> ? 2 : 4) << std::setfill('0')
        << static_cast<unsigned>(value) << "'";
    return oss.str();
  }
}
std::string get_test_name() {
  auto now = std::chrono::system_clock::now();
  return std::format("TEST(issue_{:%Y%m%d%H%M}) {{\n", now);
}

/*
 * decodes the base64 coded input
 */
template <typename FromChar, bool atomic>
decode_result
decode_impl(std::span<const FromChar> base64_, const auto selected_option,
            const std::size_t decode_buf_size, const auto last_chunk_option,
            const bool decode_up_to_bad_char) {
  std::vector<FromChar> base64(begin(base64_), end(base64_));
  std::size_t outlen = decode_buf_size;

  decode_result ret;
  ret.binary.resize(decode_buf_size);
  if constexpr (atomic) {
    ret.result = simdutf::atomic_base64_to_binary_safe(
        base64.data(), base64.size(), ret.binary.data(), outlen,
        selected_option, last_chunk_option, decode_up_to_bad_char);
  } else {
    ret.result = simdutf::base64_to_binary_safe(
        base64.data(), base64.size(), ret.binary.data(), outlen,
        selected_option, last_chunk_option, decode_up_to_bad_char);
  }

  // the number of written bytes must always be less than the supplied buffer
  assert(outlen <= decode_buf_size);

  switch (ret.result.error) {
  case simdutf::error_code::OUTPUT_BUFFER_TOO_SMALL: {
    if (!(ret.result.count <= base64.size())) {
      std::cerr << " decode_buf_size=" << decode_buf_size
                << " outlen=" << outlen << " and result=" << ret.result << '\n';
      std::abort();
    }
  } break;
  case simdutf::error_code::INVALID_BASE64_CHARACTER: {
    assert(ret.result.count < base64.size());
  } break;
  case simdutf::error_code::BASE64_INPUT_REMAINDER: {
    if (!(ret.result.count <= base64.size())) {
      std::cerr << "on input with size=" << base64.size()
                << ": got BASE64_INPUT_REMAINDER decode_buf_size="
                << decode_buf_size << " outlen=" << outlen
                << " and result=" << ret.result << '\n';
      std::abort();
    }
  } break;
  default:;
  }

  // strip away the part that was not written to (this is a temporary workaround
  // to not stop the fuzzing when base64_to_binary_safe writes outside what it
  // reports in outlen)
  ret.binary.resize(outlen);

  return ret;
}

[[nodiscard]] std::uint64_t compute_hash(const auto& data) noexcept {
  constexpr std::uint64_t fnv_prime = 1099511628211ULL;
  constexpr std::uint64_t fnv_offset = 14695981039346656037ULL;

  std::uint64_t hash = fnv_offset;
  for (const auto& item : data) {
    hash ^= static_cast<std::uint64_t>(item);
    hash *= fnv_prime;
  }
  return hash;
}

// For debugging purposes, we want to see a careful comparison of the
// output of the two implementations.
bool compare_decode_verbose(
    const auto& b64_input, const std::size_t decodesize,
    const simdutf::base64_options options,
    const simdutf::last_chunk_handling_options last_chunk_options,
    const bool decode_up_to_bad_char) {
  std::cerr << "// input size: " << b64_input.size() << "\n";
  std::cerr << "// decode buffer size: " << decodesize << "\n";
  std::cerr << "// options: " << options << "\n";
  std::cerr << "// last chunk options: " << last_chunk_options << "\n";
  std::cerr << "// decode up to bad char: " << decode_up_to_bad_char << "\n";
  std::cerr << "// hash: " << compute_hash(b64_input) << "\n";
  std::cerr << "// implementation tested: "
            << simdutf::get_active_implementation()->name() << "\n";
  std::cerr << "// ";
  for (std::size_t i = 0; i < b64_input.size(); ++i) {
    std::cerr << uint64_t(b64_input[i]) << ", ";
    if ((i + 1) % 16 == 0) {
      std::cerr << "\n";
      std::cerr << "// ";
    }
  }
  std::cerr << "\n";

  const auto s = [&]() {
    if constexpr (sizeof(b64_input[0]) == 1) {
      return std::span<const char>(
          reinterpret_cast<const char*>(b64_input.data()), b64_input.size());
    } else {
      return std::span<const char16_t>(
          reinterpret_cast<const char16_t*>(b64_input.data()),
          b64_input.size());
    }
  }();

  {
    // We are going to compute the 'true' answer.
    std::vector<char> largebuffer(s.size());
    simdutf::full_result tr =
        simdutf::get_active_implementation()->base64_to_binary_details(
            s.data(), s.size(), largebuffer.data(), options,
            last_chunk_options);
    std::cerr << "// 'correct' output " << tr.output_count << " bytes\n";
    std::cerr << "// 'correct' consumes " << tr.input_count << " characters\n";
    std::cerr << "// 'correct' has error " << tr.error << "\n";
  }

  std::vector<char> outbuf1(decodesize);
  std::size_t outlen1 = outbuf1.size();
  const auto r1 = simdutf::base64_to_binary_safe(
      s.data(), s.size(), outbuf1.data(), outlen1, options, last_chunk_options,
      decode_up_to_bad_char);
  // Check that the output is zeroed out
  for (std::size_t i = outlen1; i < decodesize; ++i) {
    if (uint8_t(outbuf1.at(i)) != 0) {
      return false;
    }
  }
  std::cerr << "// regular safe produces " << outlen1 << " bytes\n";
  std::cerr << "// regular safe consumes " << r1.count << " characters\n";
  std::cerr << "// regular has error " << r1.error << "\n";
  if (r1.error == simdutf::error_code::INVALID_BASE64_CHARACTER) {
    std::cerr << "// regular has error INVALID_BASE64_CHARACTER\n";
    if (r1.count < s.size()) {
      std::cerr << "// at chararacter " << get_code(s[r1.count]) << "\n";
    }
  }
  if (r1.error == simdutf::error_code::BASE64_INPUT_REMAINDER) {
    std::cerr << "// regular has error BASE64_INPUT_REMAINDER\n";
  }
  if (r1.error == simdutf::error_code::OUTPUT_BUFFER_TOO_SMALL) {
    std::cerr << "// regular has error OUTPUT_BUFFER_TOO_SMALL\n";
  }
  if (r1.error == simdutf::error_code::SUCCESS) {
    std::cerr << "// regular has error SUCCESS\n";
  }
  std::vector<char> outbuf2(decodesize);
  const auto [r2, outlen2] = simdutf::atomic_base64_to_binary_safe(
      s, outbuf2, options, last_chunk_options, decode_up_to_bad_char);
  for (std::size_t i = outlen2; i < decodesize; ++i) {
    if (uint8_t(outbuf2.at(i)) != 0) {
      return false;
    }
  }
  std::cerr << "// atomic produces " << outlen2 << " bytes\n";
  std::cerr << "// atomic consumes " << r2.count << " characters\n";
  std::cerr << "// atomic has error " << r2.error << "\n";
  if (r2.error == simdutf::error_code::INVALID_BASE64_CHARACTER) {
    std::cerr << "// atomic has error INVALID_BASE64_CHARACTER\n";
    if (r2.count < s.size()) {
      std::cerr << "// at chararacter " << get_code(s[r2.count]) << "\n";
    }
  }
  if (r2.error == simdutf::error_code::BASE64_INPUT_REMAINDER) {
    std::cerr << "// atomic has error BASE64_INPUT_REMAINDER\n";
  }
  if (r2.error == simdutf::error_code::OUTPUT_BUFFER_TOO_SMALL) {
    std::cerr << "// atomic has error OUTPUT_BUFFER_TOO_SMALL\n";
  }
  if (r2.error == simdutf::error_code::SUCCESS) {
    std::cerr << "// atomic has error SUCCESS\n";
  }

  // Both must agree on the kind of error
  if (decode_up_to_bad_char) {
    if (r1.error != r2.error) {
      return false;
    }
  } else {
    if ((r1.error == simdutf::error_code::SUCCESS) !=
        (r2.error == simdutf::error_code::SUCCESS)) {
      return false;
    }
  }

  // On success, must agree on the output
  if (r1.error == simdutf::error_code::SUCCESS) {
    if (outlen1 != outlen2 || r1.count != r2.count) {
      return false;
    }
    for (std::size_t i = 0; i < outlen1; ++i) {
      if (+outbuf1.at(i) != +outbuf2.at(i)) {
        return false;
      }
    }
    // Ensure remainder of the output is equal
    for (std::size_t i = outlen1; i < decodesize; ++i) {
      if (+outbuf1.at(i) != +outbuf2.at(i)) {
        return false;
      }
    }
  }
  return true;
}

template <typename FromChar>
void decode(std::span<const FromChar> base64_, const auto selected_option,
            const std::size_t decode_buf_size, const auto last_chunk_option,
            const bool decode_up_to_bad_char) {

  const auto atomic =
      decode_impl<FromChar, true>(base64_, selected_option, decode_buf_size,
                                  last_chunk_option, decode_up_to_bad_char);
  const auto non_atomic =
      decode_impl<FromChar, false>(base64_, selected_option, decode_buf_size,
                                   last_chunk_option, decode_up_to_bad_char);

  bool bad = false;
  // When decode_up_to_bad_char is true, the error code should be consistent
  if (decode_up_to_bad_char) {
    if (atomic.result.error != non_atomic.result.error) {
      std::cerr << "different error reported! " << atomic.result.error << " vs "
                << non_atomic.result.error << '\n';
      bad = true;
    }
  } else {
    // When decode_up_to_bad_char is false, they either both succeed or
    // both fail, although the error codes may differ.
    if ((atomic.result.error == simdutf::SUCCESS) !=
        (non_atomic.result.error == simdutf::SUCCESS)) {
      std::cerr << "different error reported! " << atomic.result.error << " vs "
                << non_atomic.result.error << '\n';
      bad = true;
    }
  }
  // When they both succeed, the count should be the same.
  if ((atomic.result.error == simdutf::SUCCESS) &&
      (non_atomic.result.error == simdutf::SUCCESS)) {
    if (atomic.result.count != non_atomic.result.count) {
      std::cerr << "different count reported! " << atomic.result.count << " vs "
                << non_atomic.result.count << '\n';
      bad = true;
    } else if (atomic.binary.size() != non_atomic.binary.size()) {
      if (non_atomic.result.error == simdutf::SUCCESS) {
        // suppress this output, it happens all the time otherwise
        std::cerr << "different data size! " << atomic.binary.size() << " vs "
                  << non_atomic.binary.size() << '\n';
        bad = true;
      }
    } else if (atomic.binary != non_atomic.binary) {
      std::cerr << "different data content! (but same size)\n";
      for (std::size_t i = 0; i < non_atomic.binary.size(); ++i) {
        const auto e1 = non_atomic.binary.at(i);
        const auto e2 = atomic.binary.at(i);
        if (e1 != e2) {
          std::cerr << "non_atomic[" << i << "]=" << +e1 << " != atomic[" << i
                    << "]=" << +e2 << "\n";
        }
        ++i;
      }
      bad = true;
    }
  }

  if (!bad) {
    return;
  }

  std::cerr << get_test_name();
  bool is_ok = compare_decode_verbose(base64_, decode_buf_size, selected_option,
                                      last_chunk_option, decode_up_to_bad_char);
  std::cerr << "// implementation tested: "
            << simdutf::get_active_implementation()->name() << "\n";
  if (is_ok) {
    std::cerr << "// MIGHT NOT BE A FAILURE?????\n";
  } else {
    std::cerr << "// FAILURE VERIFIED\n";
  }
  constexpr bool is_one_byte = (sizeof(FromChar) == 1);
  if (is_one_byte) {
    std::cerr << "// input is char\n";
  } else {
    std::cerr << "// input is char16_t\n";
  }
  if constexpr (is_one_byte) {

    std::cerr << "// input:\n";
    std::cerr << "// ";
    size_t count = 0;

    for (auto b : base64_) {
      count++;
      if (count % 128 == 0) {
        std::cerr << "\n// ";
      }
      if (b >= 0x20 && b < 0x7f) {
        std::cerr << static_cast<char>(b);
      } else if (b == '\n' || b == '\r' || b == '\t' || b == '\f' || b == ' ') {
        std::cerr << " ";
      } else {
        std::cerr << "!";
      }
    }
    std::cerr << "\n";
    std::cerr << "// count=" << count << "\n";
  }

  std::cerr << "const std::vector<" << (is_one_byte ? "char" : "char16_t")
            << "> base64{";
  for (auto b : base64_) {
    std::cerr << get_code(b) << ", ";
  }
  std::cerr << "};\n";
  std::cerr << "compare_decode(base64, " << std::dec << decode_buf_size
            << ", simdutf::" << NAMEOF_ENUM(selected_option) << ",\n";
  std::cerr << "simdutf::last_chunk_handling_options::"
            << NAMEOF_ENUM(last_chunk_option) << ", "
            << (decode_up_to_bad_char ? "true" : "false") << ");\n";

  std::cerr << "ASSERT_TRUE(compare_decode_verbose(base64, " << std::dec
            << decode_buf_size << ", simdutf::" << NAMEOF_ENUM(selected_option)
            << ",\n";
  std::cerr << "simdutf::last_chunk_handling_options::"
            << NAMEOF_ENUM(last_chunk_option) << ", "
            << (decode_up_to_bad_char ? "true" : "false") << "));\n";
  std::cerr << "};\n";
  // std::abort();
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  // pick one of the function pointers, based on the fuzz data
  // the first byte is which action to take. step forward
  // several bytes so the input is aligned.
  constexpr auto optionbytes = 6u;
  static_assert(optionbytes % 2 == 0,
                "optionbytes must be even to avoid misaligned char16 pointers");

  if (size < optionbytes) {
    return 0;
  }
  constexpr auto Ncases = 2u;
  constexpr auto actionmask = std::bit_ceil(Ncases) - 1;
  const auto action = data[0] & actionmask;

  // pick a random option
  const auto selected_option = [](auto index) {
    if (index >= options.size())
      return options[0];
    else {
      return options[index];
    }
  }(data[1] & (std::bit_ceil(options.size()) - 1));
  const auto selected_last_chunk =
      (selected_option == simdutf::base64_url ||
       selected_option == simdutf::base64_default_no_padding)
          ? simdutf::last_chunk_handling_options::loose
          : [](auto index) {
              if (index >= last_chunk.size())
                return last_chunk[0];
              else {
                return last_chunk[index];
              }
            }(data[2] & (std::bit_ceil(last_chunk.size()) - 1));

  // decode buffer size
  const std::size_t decode_buffer_size = (data[4] << 8) + data[3];

  const bool decode_up_to_bad_char = data[5] & 0x1;

  data += optionbytes;
  size -= optionbytes;

  switch (action) {
  case 0: {
    const std::span<const char> chardata{(const char*)data, size};
    decode(chardata, selected_option, decode_buffer_size, selected_last_chunk,
           decode_up_to_bad_char);
  } break;
  case 1: {
    const std::span<const char16_t> chardata{(const char16_t*)data, size / 2};
    decode(chardata, selected_option, decode_buffer_size, selected_last_chunk,
           decode_up_to_bad_char);
  } break;
  }

  return 0;
}
