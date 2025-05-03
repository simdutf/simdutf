#include <cstddef>
#include <cstdint>
#include <array>
#include <iomanip>
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
  if (atomic.result.error != non_atomic.result.error) {
    std::cerr << "different error reported! " << atomic.result.error << " vs "
              << non_atomic.result.error << '\n';
    bad = true;
  }

  if (atomic.result.count != non_atomic.result.count) {
    if (non_atomic.result.error == simdutf::SUCCESS) {
      std::cerr << "different count reported! " << atomic.result.count << " vs "
                << non_atomic.result.count << '\n';
      bad = true;
    }
  }
  if (atomic.binary.size() != non_atomic.binary.size()) {

    if (non_atomic.result.error == simdutf::SUCCESS) {
      // suppress this output, it happens all the time otherwise
      std::cerr << "different data size! " << atomic.binary.size() << " vs "
                << non_atomic.binary.size() << '\n';
      bad = true;
    }
  } else {
    if (non_atomic.result.error == simdutf::SUCCESS) {
      if (atomic.binary != non_atomic.binary) {
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
  }

  if (!bad) {
    return;
  }
  std::cerr << "TEST(issue_xxx) {\n";
  std::cerr << "const std::vector<"
            << (sizeof(FromChar) == 1 ? "unsigned char" : "std::uint16_t")
            << "> base64{";
  for (auto b : base64_) {
    std::cerr << "0x" << std::hex << std::setfill('0') << std::setw(2)
              << +static_cast<std::make_unsigned_t<FromChar>>(b) << ", ";
  }
  std::cerr << "};\n";
  std::cerr << "compare_decode(base64, " << std::dec << decode_buf_size
            << ", simdutf::" << NAMEOF_ENUM(selected_option) << ",\n";
  std::cerr << "simdutf::last_chunk_handling_options::"
            << NAMEOF_ENUM(last_chunk_option) << ", "
            << (decode_up_to_bad_char ? "true" : "false") << ");\n";
  std::cerr << "};\n";

  std::abort();
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
