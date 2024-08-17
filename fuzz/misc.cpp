#include <cstddef>
#include <cstdint>
#include <array>
#include <functional>

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

void utf16_endianess(std::span<const char16_t> data) {
  std::vector<std::string> results;
  const auto implementations = get_supported_implementations();
  for (const simdutf::implementation* impl : implementations) {
    std::vector<char16_t> out(data.size());
    impl->change_endianness_utf16(data.data(), data.size(), out.data());
    results.push_back(FNV1A_hash::as_str(out));
  }
  auto neq = [](const auto& a, const auto& b) { return a != b; };
  if (std::ranges::adjacent_find(results, neq) != results.end()) {
    std::cerr << "in utf16_endianess(const char*, std::size_t):\n";
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

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  // pick one of the function pointers, based on the fuzz data
  // the first byte is which action to take. step forward
  // several bytes so the input is aligned.
  if (size < 4) {
    return 0;
  }
  constexpr auto Ncases = 8u;
  constexpr auto actionmask = std::bit_ceil(Ncases) - 1;
  const auto action = data[0] & actionmask;

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
    utf16_endianess(u16data);
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
  }
  return 0;
}
