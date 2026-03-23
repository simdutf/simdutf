#include <cstddef>
#include <cstdint>
#include <ranges>

#include "helpers/common.h"
#include "simdutf.h"

void autodetect(std::span<const char> chardata) {
  std::vector<simdutf::encoding_type> results;
  const auto implementations = get_supported_implementations();
  for (const simdutf::implementation* impl : implementations) {
    results.push_back(
        impl->autodetect_encoding(chardata.data(), chardata.size()));
  }
  auto neq = [](const auto& a, const auto& b) { return a != b; };
  if (std::ranges::adjacent_find(results, neq) != results.end()) {
    std::cerr << "output differs between implementations\n";
    for (std::size_t i = 0; i < implementations.size(); ++i) {
      std::cerr << "implementation " << implementations[i] << " gave "
                << results.at(i) << '\n';
    }
    std::abort();
  }
}

void detect(std::span<const char> chardata) {
  std::vector<int> results;
  const auto implementations = get_supported_implementations();
  for (const simdutf::implementation* impl : implementations) {
    results.push_back(impl->detect_encodings(chardata.data(), chardata.size()));
  }
  auto neq = [](const auto& a, const auto& b) { return a != b; };
  if (std::ranges::adjacent_find(results, neq) != results.end()) {
    std::cerr << "in detect_encodings(const char*, std::size_t):\n";
    std::cerr << "output differs between implementations\n";
    for (std::size_t i = 0; i < implementations.size(); ++i) {
      std::cerr << "implementation " << implementations[i]->name() << " gave "
                << results.at(i) << '\n';
    }
    std::cerr << " std::vector<unsigned char> data{";
    for (unsigned char x : chardata) {
      std::cerr << +x << ", ";
    };
    std::cerr << "};\n";
    std::abort();
  }
}

void validate_ascii(std::span<const char> chardata) {
  // use int, not bool to avoid vector<bool>
  std::vector<int> results;
  const auto implementations = get_supported_implementations();
  for (const simdutf::implementation* impl : implementations) {
    results.push_back(+impl->validate_ascii(chardata.data(), chardata.size()));
  }
  auto neq = [](const auto& a, const auto& b) { return a != b; };
  if (std::ranges::adjacent_find(results, neq) != results.end()) {
    std::cerr << "in validate_ascii(const char*, std::size_t):\n";
    std::cerr << "output differs between implementations\n";
    for (std::size_t i = 0; i < implementations.size(); ++i) {
      std::cerr << "implementation " << implementations[i]->name() << " gave "
                << results.at(i) << '\n';
    }
    std::cerr << " std::vector<unsigned char> data{";
    for (unsigned char x : chardata) {
      std::cerr << +x << ", ";
    };
    std::cerr << "};\n";
    std::abort();
  }
}

void validate_ascii_with_err(std::span<const char> chardata) {
  // use int, not bool to avoid vector<bool>
  std::vector<simdutf::result> results;
  const auto implementations = get_supported_implementations();
  for (const simdutf::implementation* impl : implementations) {
    results.push_back(
        impl->validate_ascii_with_errors(chardata.data(), chardata.size()));
  }
  auto neq = [](const auto& a, const auto& b) { return a != b; };
  if (std::ranges::adjacent_find(results, neq) != results.end()) {
    std::cerr << "in validate_ascii(const char*, std::size_t):\n";
    std::cerr << "output differs between implementations\n";
    for (std::size_t i = 0; i < implementations.size(); ++i) {
      std::cerr << "implementation " << implementations[i]->name() << " gave "
                << results.at(i) << '\n';
    }
    std::cerr << " std::vector<unsigned char> data{";
    for (unsigned char x : chardata) {
      std::cerr << +x << ", ";
    };
    std::cerr << "};\n";
    std::abort();
  }
}

void utf16_endianness(std::span<const char16_t> data) {
  std::vector<std::string> results;
  const auto implementations = get_supported_implementations();
  for (const simdutf::implementation* impl : implementations) {
    std::vector<char16_t> out(data.size());
    impl->change_endianness_utf16(data.data(), data.size(), out.data());
    results.push_back(FNV1A_hash::as_str(out));
  }
  auto neq = [](const auto& a, const auto& b) { return a != b; };
  if (std::ranges::adjacent_find(results, neq) != results.end()) {
    std::cerr << "in utf16_endianness(const char*, std::size_t):\n";
    std::cerr << "output differs between implementations\n";
    for (std::size_t i = 0; i < implementations.size(); ++i) {
      std::cerr << "implementation " << implementations[i]->name() << " gave "
                << results.at(i) << '\n';
    }
    std::cerr << " std::vector<char16_t> data{";
    for (int x : data) {
      std::cerr << +x << ", ";
    };
    std::cerr << "};\n";
    std::abort();
  }
}

// Checks that validate_utf16le_as_ascii and validate_utf16be_as_ascii agree
// across all implementations, and that a true result implies valid UTF-16.
void validate_utf16_as_ascii(std::span<const char16_t> data) {
  const auto implementations = get_supported_implementations();
  // use int, not bool to avoid vector<bool>
  std::vector<int> le_results, be_results;
  le_results.reserve(implementations.size());
  be_results.reserve(implementations.size());
  for (const simdutf::implementation* impl : implementations) {
    le_results.push_back(+impl->validate_utf16le_as_ascii(data.data(), data.size()));
    be_results.push_back(+impl->validate_utf16be_as_ascii(data.data(), data.size()));
  }
  auto neq = [](const auto& a, const auto& b) { return a != b; };
  if (std::ranges::adjacent_find(le_results, neq) != le_results.end()) {
    std::cerr << "validate_utf16le_as_ascii: output differs between implementations\n";
    for (std::size_t i = 0; i < implementations.size(); ++i) {
      std::cerr << "  " << implementations[i]->name() << " gave " << le_results[i] << '\n';
    }
    std::abort();
  }
  if (std::ranges::adjacent_find(be_results, neq) != be_results.end()) {
    std::cerr << "validate_utf16be_as_ascii: output differs between implementations\n";
    for (std::size_t i = 0; i < implementations.size(); ++i) {
      std::cerr << "  " << implementations[i]->name() << " gave " << be_results[i] << '\n';
    }
    std::abort();
  }
  // If LE validates as ASCII, it must also validate as UTF-16LE (ASCII is a subset).
  if (le_results[0]) {
    for (const simdutf::implementation* impl : implementations) {
      if (!impl->validate_utf16le(data.data(), data.size())) {
        std::cerr << "validate_utf16le_as_ascii returned true but validate_utf16le returned false"
                  << " impl=" << impl->name() << "\n";
        std::abort();
      }
    }
  }
  // Same for BE.
  if (be_results[0]) {
    for (const simdutf::implementation* impl : implementations) {
      if (!impl->validate_utf16be(data.data(), data.size())) {
        std::cerr << "validate_utf16be_as_ascii returned true but validate_utf16be returned false"
                  << " impl=" << impl->name() << "\n";
        std::abort();
      }
    }
  }
}

// Checks that to_well_formed_utf16le / to_well_formed_utf16be:
// 1. All implementations agree on the output.
// 2. The output is always valid UTF-16LE / UTF-16BE.
// 3. When the input is already valid UTF-16, the output equals the input.
void to_well_formed_utf16(std::span<const char16_t> data) {
  const auto implementations = get_supported_implementations();
  if (implementations.empty()) { return; }

  // Check LE variant
  {
    std::vector<std::vector<char16_t>> le_outputs;
    le_outputs.reserve(implementations.size());
    for (const simdutf::implementation* impl : implementations) {
      std::vector<char16_t> out(data.size());
      impl->to_well_formed_utf16le(data.data(), data.size(), out.data());
      le_outputs.push_back(std::move(out));
    }
    auto neq = [](const auto& a, const auto& b) { return a != b; };
    if (std::ranges::adjacent_find(le_outputs, neq) != le_outputs.end()) {
      std::cerr << "to_well_formed_utf16le: outputs differ between implementations\n";
      for (std::size_t i = 0; i < implementations.size(); ++i) {
        std::cerr << "  " << implementations[i]->name()
                  << ": hash=" << FNV1A_hash::as_str(le_outputs[i]) << "\n";
      }
      std::abort();
    }
    // Output must be valid UTF-16LE.
    for (std::size_t i = 0; i < implementations.size(); ++i) {
      if (!implementations[i]->validate_utf16le(le_outputs[i].data(), le_outputs[i].size())) {
        std::cerr << "to_well_formed_utf16le: output is not valid UTF-16LE"
                  << " impl=" << implementations[i]->name() << "\n";
        std::abort();
      }
    }
    // If input was already valid UTF-16LE, output must equal input.
    if (implementations[0]->validate_utf16le(data.data(), data.size())) {
      if (!std::ranges::equal(le_outputs[0], data)) {
        std::cerr << "to_well_formed_utf16le: valid input was modified\n";
        std::abort();
      }
    }
  }

  // Check BE variant
  {
    std::vector<std::vector<char16_t>> be_outputs;
    be_outputs.reserve(implementations.size());
    for (const simdutf::implementation* impl : implementations) {
      std::vector<char16_t> out(data.size());
      impl->to_well_formed_utf16be(data.data(), data.size(), out.data());
      be_outputs.push_back(std::move(out));
    }
    auto neq = [](const auto& a, const auto& b) { return a != b; };
    if (std::ranges::adjacent_find(be_outputs, neq) != be_outputs.end()) {
      std::cerr << "to_well_formed_utf16be: outputs differ between implementations\n";
      for (std::size_t i = 0; i < implementations.size(); ++i) {
        std::cerr << "  " << implementations[i]->name()
                  << ": hash=" << FNV1A_hash::as_str(be_outputs[i]) << "\n";
      }
      std::abort();
    }
    // Output must be valid UTF-16BE.
    for (std::size_t i = 0; i < implementations.size(); ++i) {
      if (!implementations[i]->validate_utf16be(be_outputs[i].data(), be_outputs[i].size())) {
        std::cerr << "to_well_formed_utf16be: output is not valid UTF-16BE"
                  << " impl=" << implementations[i]->name() << "\n";
        std::abort();
      }
    }
    // If input was already valid UTF-16BE, output must equal input.
    if (implementations[0]->validate_utf16be(data.data(), data.size())) {
      if (!std::ranges::equal(be_outputs[0], data)) {
        std::cerr << "to_well_formed_utf16be: valid input was modified\n";
        std::abort();
      }
    }
  }
}

void convert_latin1_to_utf8_safe(std::span<const char> chardata,
                                 const std::size_t outputsize) {
  // convert with a limited output buffer
  std::vector<char> limited_output(outputsize);
  const auto limited_ret = simdutf::convert_latin1_to_utf8_safe(
      chardata.data(), chardata.size(), limited_output.data(), outputsize);

  // convert with a sufficiently large output buffer
  std::vector<char> large_output(2 * chardata.size());
  const auto large_ret = simdutf::convert_latin1_to_utf8(
      chardata.data(), chardata.size(), large_output.data());

  if (large_ret != 0) {
    // conversion was possible with a large buffer.
    if (large_ret <= outputsize) {
      // the limited buffer was large enough, ensure we got the same result
      assert(limited_ret == large_ret);
      assert(std::ranges::equal(limited_output | std::views::take(large_ret),
                                large_output | std::views::take(large_ret)));
    } else {
      // the number of written bytes for a limited buffer must not exceed what
      // the large buffer got.
      assert(limited_ret <= large_ret);
      // the written data should be equal
      assert(std::ranges::equal(limited_output | std::views::take(limited_ret),
                                large_output | std::views::take(limited_ret)));
    }
  } else {
    // conversion with a big buffer failed - is there anything we can check or
    // assert for the limited buffer? I don't think so.
  }
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  // pick one of the functions, based on the fuzz data.
  // the first byte is which action to take. step forward
  // several bytes so the input is aligned.
  if (size < 4) {
    return 0;
  }
  constexpr auto Ncases = 11u;
  constexpr auto actionmask = std::bit_ceil(Ncases) - 1;
  const auto action = data[0] & actionmask;

  const std::uint16_t u16 = data[1] + (data[2] << 8);

  data += 4;
  size -= 4;

  const std::span<const char> chardata{(const char*)data, size};
  const std::span<const char16_t> u16data{(const char16_t*)data,
                                          size / sizeof(char16_t)};

  switch (action) {
  case 0:
    autodetect(chardata);
    break;
  case 1:
    detect(chardata);
    break;
  case 2:
    validate_ascii(chardata);
    break;
  case 3:
    validate_ascii_with_err(chardata);
    break;
  case 4:
    utf16_endianness(u16data);
    break;
  case 5: {
    [[maybe_unused]] auto ret =
        simdutf::trim_partial_utf16le(u16data.data(), u16data.size());
    assert(ret == u16data.size() || ret + 1 == u16data.size());
  } break;
  case 6: {
    [[maybe_unused]] auto ret =
        simdutf::trim_partial_utf16be(u16data.data(), u16data.size());
    assert(ret == u16data.size() || ret + 1 == u16data.size());
  } break;
  case 7: {
    [[maybe_unused]] const std::size_t N = chardata.size();
    [[maybe_unused]] const auto ret =
        simdutf::trim_partial_utf8(chardata.data(), chardata.size());
    if ((ret + 3 < N) || (ret > N)) {
      std::cerr << "ret=" << ret << " N=" << N << '\n';
      std::abort();
    }
  } break;
  case 8:
    convert_latin1_to_utf8_safe(chardata, u16);
    break;
  case 9:
    validate_utf16_as_ascii(u16data);
    break;
  case 10:
    to_well_formed_utf16(u16data);
    break;
  }
  return 0;
}
