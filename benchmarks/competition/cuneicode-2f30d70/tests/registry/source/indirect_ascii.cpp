// ============================================================================
//
// ztd.cuneicode
// Copyright © 2022-2022 JeanHeyd "ThePhD" Meneide and Shepherd's Oasis, LLC
// Contact: opensource@soasis.org
//
// Commercial License Usage
// Licensees holding valid commercial ztd.cuneicode licenses may use this file
// in accordance with the commercial license agreement provided with the
// Software or, alternatively, in accordance with the terms contained in
// a written agreement between you and Shepherd's Oasis, LLC.
// For licensing terms and conditions see your agreement. For
// further information contact opensource@soasis.org.
//
// Apache License Version 2 Usage
// Alternatively, this file may be used under the terms of Apache License
// Version 2.0 (the "License"); you may not use this file except in compliance
// with the License. You may obtain a copy of the License at
//
// 		http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// ========================================================================= //

#include <catch2/catch_all.hpp>

#include <ztd/cuneicode.h>

#include <ztd/idk/size.hpp>
#include <ztd/idk/encoding_name.hpp>

#include <iterator>
#include <algorithm>
#include <cstring>
#include <string_view>
#include <tuple>

inline namespace cnc_tests_registry_indirect_ascii {

	template <typename Source, typename PivotExpected, typename Expected>
	void compare_registry_roundtrip(cnc_conversion* conv, const Source& source,
	     const PivotExpected& pivot_expected, const Expected& expected) {
		unsigned char pivot_buffer[500] = {};
		cnc_pivot_info pivot = { ztd_c_array_size(pivot_buffer), pivot_buffer, CNC_MCERROR_OK };
		alignas(ztd_char32_t) unsigned char output_buffer[500] = {};
		unsigned char* output                                  = output_buffer;
		size_t output_size                                     = ztd_c_array_size(output_buffer);
		const unsigned char* input = (const unsigned char*)source.data();
		size_t input_size          = source.size();
		cnc_mcerror err
		     = cnc_conv_pivot(conv, &output_size, &output, &input_size, &input, &pivot);
		std::string_view pivot_view((const ztd_char_t*)pivot.bytes, pivot_expected.size());
		std::u32string_view output_view((const ztd_char32_t*)output_buffer,
		     (ztd_c_array_size(output_buffer) - output_size) / sizeof(ztd_char32_t));
		const bool err_okay    = err == CNC_MCERROR_OK;
		const bool pivot_okay  = pivot_view == pivot_expected;
		const bool output_okay = output_view == expected;
		REQUIRE(err_okay);
		REQUIRE(pivot_okay);
		REQUIRE(output_okay);
	}

	struct cnc_conversion_deleter {
		void operator()(cnc_conversion* handle) const {
			cnc_conv_delete(handle);
		}
	};

	struct cnc_registry_deleter {
		void operator()(cnc_conversion_registry* handle) const {
			cnc_registry_delete(handle);
		}
	};

	cnc_mcerror mcnrtomcn_weird1_ascii(cnc_conversion*, size_t* __p_maybe_dst_len,
	     unsigned char** __p_maybe_dst, size_t* __p_src_len, const unsigned char** __p_src,
	     cnc_pivot_info*, void*) {
		if (__p_src_len == nullptr || __p_src == nullptr) {
			return CNC_MCERROR_OK;
		}

		const bool _IsUnbounded     = __p_maybe_dst_len == nullptr;
		const bool _IsCounting      = __p_maybe_dst == nullptr || *__p_maybe_dst == nullptr;
		size_t& __src_len           = *__p_src_len;
		const unsigned char*& __src = *__p_src;
		if (__src_len == 0 || __src == nullptr) {
			return CNC_MCERROR_OK;
		}
		if (!_IsUnbounded) {
			if (*__p_maybe_dst_len < 1) {
				return CNC_MCERROR_INSUFFICIENT_OUTPUT;
			}
		}
		const unsigned char __c0 = *__src;
		if (__c0 > 0x7F) {
			return CNC_MCERROR_INVALID_SEQUENCE;
		}
		__src += 1;
		__src_len -= 1;
		if (!_IsCounting) {
			**__p_maybe_dst = static_cast<unsigned char>(__c0);
			*__p_maybe_dst += 1;
		}
		if (!_IsUnbounded) {
			*__p_maybe_dst_len -= 1;
		}
		return CNC_MCERROR_OK;
	}

	cnc_mcerror mcnrtomcn_ascii_weird1(cnc_conversion*, size_t* __p_maybe_dst_len,
	     unsigned char** __p_maybe_dst, size_t* __p_src_len, const unsigned char** __p_src,
	     cnc_pivot_info*, void*) {
		if (__p_src_len == nullptr || __p_src == nullptr) {
			return CNC_MCERROR_OK;
		}

		const bool _IsUnbounded     = __p_maybe_dst_len == nullptr;
		const bool _IsCounting      = __p_maybe_dst == nullptr || *__p_maybe_dst == nullptr;
		size_t& __src_len           = *__p_src_len;
		const unsigned char*& __src = *__p_src;
		if (__src_len == 0 || __src == nullptr) {
			return CNC_MCERROR_OK;
		}
		if (!_IsUnbounded) {
			if (*__p_maybe_dst_len < 1) {
				return CNC_MCERROR_INSUFFICIENT_OUTPUT;
			}
		}
		const unsigned char __c0 = *__src;
		if (__c0 > 0x7F) {
			return CNC_MCERROR_INVALID_SEQUENCE;
		}
		__src += 1;
		__src_len -= 1;
		if (!_IsCounting) {
			**__p_maybe_dst = static_cast<unsigned char>(__c0);
			*__p_maybe_dst += 1;
		}
		if (!_IsUnbounded) {
			*__p_maybe_dst_len -= 1;
		}
		return CNC_MCERROR_OK;
	}

} // namespace cnc_tests_registry_indirect_ascii


TEST_CASE("conversion from a custom encoding to UTF-32, through the ASCII encoding",
     "[cuneicode][registry][indirect][non-unicode]") {
	const std::tuple<std::string_view, std::string_view, std::u32string_view>
	     source_and_expected_bundles[] = {
		     { "abcdefghijklmnopqrstuvwxyz", "abcdefghijklmnopqrstuvwxyz",
		          U"abcdefghijklmnopqrstuvwxyz" },
	     };
	// open the registry
	std::unique_ptr<cnc_conversion_registry, cnc_registry_deleter> registry = nullptr;
	{
		cnc_conversion_registry* raw_registry = registry.get();
		cnc_registry_options registry_options = CNC_REGISTRY_OPTIONS_DEFAULT;
		cnc_open_error err                    = cnc_registry_new(&raw_registry, registry_options);
		REQUIRE(err == CNC_OPEN_ERROR_OK);
		registry.reset(raw_registry);
	}
	// add new conversion from our (weird) encoding to ASCII, then ASCII to UTF-32
	{
		cnc_open_error from_err = cnc_registry_add_c8_single(registry.get(),
		     (const ztd_char8_t*)u8"weird-1", (const ztd_char8_t*)u8"ascii",
		     mcnrtomcn_weird1_ascii, nullptr, nullptr, nullptr);
		REQUIRE(from_err == CNC_OPEN_ERROR_OK);
		cnc_open_error to_err = cnc_registry_add_c8_single(registry.get(),
		     (const ztd_char8_t*)u8"ascii", (const ztd_char8_t*)u8"weird-1",
		     mcnrtomcn_ascii_weird1, nullptr, nullptr, nullptr);
		REQUIRE(to_err == CNC_OPEN_ERROR_OK);
	} // open the should-be-direct conversion
	std::unique_ptr<cnc_conversion, cnc_conversion_deleter> conversion = nullptr;
	cnc_conversion_info info                                           = {};
	{
		std::string_view ascii_name("ascii");
		std::string_view weird1_name("weird-1");
		std::string_view utf32_name("UTF-32");
		cnc_conversion* raw_conversion      = conversion.get();
		std::size_t from_size               = 7;
		const ztd_char8_t* from_data        = (const ztd_char8_t*)weird1_name.data();
		std::size_t to_size                 = 6;
		const ztd_char8_t* to_data          = (const ztd_char8_t*)utf32_name.data();
		cnc_conversion_registry* __registry = registry.get();
		cnc_open_error err                  = cnc_conv_new_c8n(
               __registry, from_size, from_data, to_size, to_data, &raw_conversion, &info);
		REQUIRE(err == CNC_OPEN_ERROR_OK);
		REQUIRE(info.is_indirect);
		std::string_view from_name((const char*)info.from_code_data, info.from_code_size);
		std::string_view indirect_name(
		     (const char*)info.indirect_code_data, info.indirect_code_size);
		std::string_view to_name((const char*)info.to_code_data, info.to_code_size);
		REQUIRE(ztd::is_encoding_name_equal_for(ascii_name, indirect_name));
		REQUIRE(ztd::is_encoding_name_equal(weird1_name, from_name));
		REQUIRE(ztd::is_encoding_name_equal(utf32_name, to_name));
		conversion.reset(raw_conversion);
	}

	for (const auto& source_and_expected : source_and_expected_bundles) {
		const std::string_view& source         = std::get<0>(source_and_expected);
		const std::string_view& pivot_expected = std::get<1>(source_and_expected);
		const std::u32string_view& expected    = std::get<2>(source_and_expected);
		compare_registry_roundtrip(conversion.get(), source, pivot_expected, expected);
	}
}
