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

#include <ztd/tests/basic_unicode_strings.hpp>
#include <ztd/idk/detail/unicode.hpp>

#include <iterator>
#include <algorithm>
#include <cstring>

namespace {

	template <typename Char, bool IsUnicode = true>
	void compare_roundtrip_utf32(cnc_conversion* from_conv, cnc_conversion* to_conv) {
		for (ztd_char32_t expected_c = 0; expected_c < __ztd_idk_detail_last_unicode_code_point;
		     ++expected_c) {
			if constexpr (IsUnicode) {
				if (__ztd_idk_detail_is_surrogate(expected_c)) {
					continue;
				}
			}
			else {
				if (ztd::tests::u32_basic_source_character_set.find(expected_c)
				     == std::u32string_view::npos) {
					continue;
				}
			}
			using output_char              = Char;
			using input_char               = ztd_char32_t;
			unsigned char input_buffer[64] = {};
			std::memcpy(input_buffer, &expected_c, sizeof(expected_c));
			REQUIRE(std::memcmp(input_buffer, &expected_c, sizeof(expected_c)) == 0);
			const unsigned char* initial_input = input_buffer;
			const unsigned char* input         = initial_input;
			const size_t initial_input_size    = 64;
			size_t input_size                  = initial_input_size;
			unsigned char output_buffer[64];
			unsigned char* initial_output    = output_buffer;
			unsigned char* output            = initial_output;
			const size_t initial_output_size = 64;
			size_t output_size               = initial_output_size;

			const cnc_mcerror output_err = cnc_conv_one(from_conv, &output_size,
			     (unsigned char**)&output, &input_size, (const unsigned char**)&input);
			const size_t input_read      = initial_input_size - input_size;
			const size_t output_written  = initial_output_size - output_size;
			REQUIRE(output_err == CNC_MCERROR_OK);
			REQUIRE((input - initial_input)
			     == (static_cast<std::ptrdiff_t>(1 * sizeof(input_char))));
			REQUIRE(input_read == (1 * sizeof(input_char)));
			REQUIRE((output - initial_output)
			     >= (static_cast<std::ptrdiff_t>(0 * sizeof(output_char))));
			REQUIRE(((output - initial_output) % sizeof(output_char)) == 0);

			{
				// validate input
				const unsigned char* initial_validate_input = initial_input;
				const unsigned char* validate_input         = initial_validate_input;
				size_t validate_input_size                  = initial_input_size;
				const cnc_mcerror validate_input_err        = cnc_conv_one(
				            from_conv, nullptr, nullptr, &validate_input_size, &validate_input);
				REQUIRE(validate_input_err == CNC_MCERROR_OK);
				REQUIRE((validate_input - initial_input)
				     == (static_cast<std::ptrdiff_t>(input_read)));
			}
			{
				// validate input: direct
				const unsigned char* initial_validate_input = initial_input;
				const unsigned char* validate_input         = initial_validate_input;
				size_t validate_input_size                  = initial_input_size;
				const bool validate_input_is_valid
				     = cnc_conv_one_is_valid(from_conv, &validate_input_size, &validate_input);
				REQUIRE(validate_input_is_valid);
				REQUIRE((validate_input - initial_input)
				     == (static_cast<std::ptrdiff_t>(input_read)));
			}
			{
				// count output
				const unsigned char* initial_count_input = initial_input;
				const unsigned char* count_input         = initial_count_input;
				size_t count_input_size                  = initial_input_size;
				size_t count_output_size                 = initial_output_size;
				const cnc_mcerror count_input_err        = cnc_conv_one(
				            from_conv, &count_output_size, nullptr, &count_input_size, &count_input);
				const size_t count_output_written = initial_output_size - count_output_size;
				REQUIRE(count_input_err == CNC_MCERROR_OK);
				REQUIRE((count_input - initial_count_input)
				     == (static_cast<std::ptrdiff_t>(input_read)));
				REQUIRE(count_output_written == output_written);
			}
			{
				// count output: direct
				const unsigned char* initial_count_input = initial_input;
				const unsigned char* count_input         = initial_count_input;
				size_t count_input_size                  = initial_input_size;
				size_t count_output_size                 = initial_output_size;
				const cnc_mcerror count_input_err        = cnc_conv_one_count(
				            from_conv, &count_output_size, &count_input_size, &count_input);
				const size_t count_output_written = initial_output_size - count_output_size;
				REQUIRE(count_input_err == CNC_MCERROR_OK);
				REQUIRE((count_input - initial_count_input)
				     == (static_cast<std::ptrdiff_t>(input_read)));
				REQUIRE(count_output_written == output_written);
			}
			{
				// unbounded output
				const unsigned char* initial_unbounded_input = initial_input;
				const unsigned char* unbounded_input         = initial_unbounded_input;
				size_t unbounded_input_size                  = initial_input_size;
				unsigned char unbounded_output_buffer[64];
				unsigned char* initial_unbounded_output = unbounded_output_buffer;
				unsigned char* unbounded_output         = initial_unbounded_output;
				const cnc_mcerror unbounded_input_err   = cnc_conv_one(from_conv, nullptr,
				       &unbounded_output, &unbounded_input_size, &unbounded_input);
				REQUIRE(unbounded_input_err == CNC_MCERROR_OK);
				REQUIRE((unbounded_input - initial_unbounded_input)
				     == (static_cast<std::ptrdiff_t>(input_read)));
				REQUIRE((unbounded_output - initial_unbounded_output)
				     == (static_cast<std::ptrdiff_t>(output_written)));
				REQUIRE((initial_unbounded_output + output_written) == unbounded_output);
				REQUIRE(std::equal(initial_unbounded_output + 0,
				     initial_unbounded_output + output_written, initial_output + 0,
				     initial_output + output_written));
			}
			{
				// unbounded output: direct
				const unsigned char* initial_unbounded_input = initial_input;
				const unsigned char* unbounded_input         = initial_unbounded_input;
				size_t unbounded_input_size                  = initial_input_size;
				unsigned char unbounded_output_buffer[64];
				unsigned char* initial_unbounded_output = unbounded_output_buffer;
				unsigned char* unbounded_output         = initial_unbounded_output;
				const cnc_mcerror unbounded_input_err   = cnc_conv_one_unbounded(
				       from_conv, &unbounded_output, &unbounded_input_size, &unbounded_input);
				REQUIRE(unbounded_input_err == CNC_MCERROR_OK);
				REQUIRE((unbounded_input - initial_unbounded_input)
				     == (static_cast<std::ptrdiff_t>(input_read)));
				REQUIRE((unbounded_output - initial_unbounded_output)
				     == (static_cast<std::ptrdiff_t>(output_written)));
				REQUIRE((initial_unbounded_output + output_written) == unbounded_output);
				REQUIRE(std::equal(initial_unbounded_output + 0,
				     initial_unbounded_output + output_written, initial_output + 0,
				     initial_output + output_written));
			}

			const unsigned char* initial_intermediate = output_buffer;
			const unsigned char* intermediate         = initial_intermediate;
			const size_t initial_intermediate_size    = output_written;
			size_t intermediate_size                  = initial_intermediate_size;
			unsigned char input_output_buffer[64];
			unsigned char* initial_input_output    = input_output_buffer;
			unsigned char* input_output            = initial_input_output;
			const size_t initial_input_output_size = 64;
			size_t input_output_size               = initial_input_output_size;
			const cnc_mcerror input_output_err     = cnc_conv_one(
			         to_conv, &input_output_size, &input_output, &intermediate_size, &intermediate);
			const size_t intermediate_read    = initial_intermediate_size - intermediate_size;
			const size_t input_output_written = initial_input_output_size - input_output_size;
			REQUIRE(input_output_err == CNC_MCERROR_OK);
			REQUIRE((intermediate - initial_intermediate)
			     == (static_cast<std::ptrdiff_t>(output_written)));
			REQUIRE(intermediate_size == 0);
			REQUIRE(intermediate_read == output_written);
			REQUIRE((input_output - initial_input_output)
			     == (static_cast<std::ptrdiff_t>(1 * sizeof(input_char))));
			REQUIRE(input_output_written == (1 * sizeof(input_char)));
			REQUIRE(input_output_written == input_read);
			REQUIRE((initial_intermediate + intermediate_read) == intermediate);
			REQUIRE((initial_intermediate_size - intermediate_read) == intermediate_size);
			REQUIRE((initial_input_output + input_output_written) == input_output);
			REQUIRE((initial_input_output_size - input_output_written) == input_output_size);
			REQUIRE(std::memcmp(input_output_buffer, &expected_c, sizeof(expected_c)) == 0);

			{
				// validate input
				const unsigned char* initial_validate_input = initial_intermediate;
				const unsigned char* validate_input         = initial_validate_input;
				size_t validate_input_size                  = initial_intermediate_size;
				const cnc_mcerror validate_input_err        = cnc_conv_one(
				            to_conv, nullptr, nullptr, &validate_input_size, &validate_input);
				REQUIRE(validate_input_err == CNC_MCERROR_OK);
				REQUIRE((validate_input - initial_validate_input)
				     == (static_cast<std::ptrdiff_t>(intermediate_read)));
			}
			{
				// validate input: direct
				const unsigned char* initial_validate_input = initial_intermediate;
				const unsigned char* validate_input         = initial_validate_input;
				size_t validate_input_size                  = initial_intermediate_size;
				const bool validate_input_is_valid
				     = cnc_conv_one_is_valid(to_conv, &validate_input_size, &validate_input);
				REQUIRE(validate_input_is_valid);
				REQUIRE((validate_input - initial_validate_input)
				     == (static_cast<std::ptrdiff_t>(intermediate_read)));
			}
			{
				// count output
				const unsigned char* initial_count_input = initial_intermediate;
				const unsigned char* count_input         = initial_count_input;
				size_t count_input_size                  = initial_intermediate_size;
				size_t count_output_size                 = initial_output_size;
				const cnc_mcerror count_input_err        = cnc_conv_one(
				            to_conv, &count_output_size, nullptr, &count_input_size, &count_input);
				const size_t count_output_written = initial_output_size - count_output_size;
				REQUIRE(count_input_err == CNC_MCERROR_OK);
				REQUIRE((count_input - initial_count_input)
				     == (static_cast<std::ptrdiff_t>(intermediate_read)));
				REQUIRE(count_output_written == input_output_written);
			}
			{
				// count output: direct
				const unsigned char* initial_count_input = initial_intermediate;
				const unsigned char* count_input         = initial_count_input;
				size_t count_input_size                  = initial_intermediate_size;
				size_t count_output_size                 = initial_output_size;
				const cnc_mcerror count_input_err        = cnc_conv_one_count(
				            to_conv, &count_output_size, &count_input_size, &count_input);
				const size_t count_output_written = initial_output_size - count_output_size;
				REQUIRE(count_input_err == CNC_MCERROR_OK);
				REQUIRE((count_input - initial_count_input)
				     == (static_cast<std::ptrdiff_t>(intermediate_read)));
				REQUIRE(count_output_written == input_output_written);
			}
			{
				// unbounded output
				const unsigned char* initial_unbounded_input = initial_intermediate;
				const unsigned char* unbounded_input         = initial_unbounded_input;
				size_t unbounded_input_size                  = initial_input_output_size;
				unsigned char unbounded_output_buffer[64];
				unsigned char* initial_unbounded_output = unbounded_output_buffer;
				unsigned char* unbounded_output         = initial_unbounded_output;
				const cnc_mcerror unbounded_input_err   = cnc_conv_one(to_conv, nullptr,
				       &unbounded_output, &unbounded_input_size, &unbounded_input);
				REQUIRE(unbounded_input_err == CNC_MCERROR_OK);
				REQUIRE((unbounded_input - initial_unbounded_input)
				     == (static_cast<std::ptrdiff_t>(intermediate_read)));
				REQUIRE((unbounded_output - initial_unbounded_output)
				     == (static_cast<std::ptrdiff_t>(input_output_written)));
				REQUIRE((initial_unbounded_output + input_output_written) == unbounded_output);
				REQUIRE(std::equal(initial_unbounded_output + 0,
				     initial_unbounded_output + input_output_written, initial_input_output + 0,
				     initial_input_output + input_output_written));
			}
			{
				// unbounded output: direct
				const unsigned char* initial_unbounded_input = initial_intermediate;
				const unsigned char* unbounded_input         = initial_unbounded_input;
				size_t unbounded_input_size                  = initial_input_output_size;
				unsigned char unbounded_output_buffer[64];
				unsigned char* initial_unbounded_output = unbounded_output_buffer;
				unsigned char* unbounded_output         = initial_unbounded_output;
				const cnc_mcerror unbounded_input_err   = cnc_conv_one_unbounded(
				       to_conv, &unbounded_output, &unbounded_input_size, &unbounded_input);
				REQUIRE(unbounded_input_err == CNC_MCERROR_OK);
				REQUIRE((unbounded_input - initial_unbounded_input)
				     == (static_cast<std::ptrdiff_t>(intermediate_read)));
				REQUIRE((unbounded_output - initial_unbounded_output)
				     == (static_cast<std::ptrdiff_t>(input_output_written)));
				REQUIRE((initial_unbounded_output + input_output_written) == unbounded_output);
				REQUIRE(std::equal(initial_unbounded_output + 0,
				     initial_unbounded_output + input_output_written, initial_input_output + 0,
				     initial_input_output + input_output_written));
			}
		}
	}
} // namespace

TEST_CASE(
     "check single conversion from one of the typical typed encodings to UTF-32 and back to the "
     "typed encoding, using the registry",
     "[cuneicode][registry][roundtrip-c32][single]") {
	cnc_conversion_registry* registry = NULL;
	cnc_open_error reg_err            = cnc_registry_new(&registry, CNC_REGISTRY_OPTIONS_DEFAULT);
	REQUIRE(reg_err == CNC_OPEN_ERROR_OK);
	SECTION("mc") {
		const ztd_char8_t* from_name  = (const ztd_char8_t*)u8"utf32";
		const ztd_char8_t* to_name    = (const ztd_char8_t*)u8"execution";
		cnc_conversion* from_conv     = NULL;
		cnc_conversion* to_conv       = NULL;
		cnc_conversion_info from_info = {};
		cnc_conversion_info to_info   = {};
		cnc_open_error from_err
		     = cnc_conv_new_c8(registry, from_name, to_name, &from_conv, &from_info);
		cnc_open_error to_err = cnc_conv_new_c8(registry, to_name, from_name, &to_conv, &to_info);
		REQUIRE(from_err == CNC_OPEN_ERROR_OK);
		REQUIRE(to_err == CNC_OPEN_ERROR_OK);
		compare_roundtrip_utf32<ztd_char_t, false>(from_conv, to_conv);
		if (cnc_is_execution_encoding_unicode()) {
			compare_roundtrip_utf32<ztd_char_t, true>(from_conv, to_conv);
		}
		cnc_conv_delete(from_conv);
		cnc_conv_delete(to_conv);
	}
	SECTION("mwc") {
		const ztd_char8_t* from_name  = (const ztd_char8_t*)u8"utf32";
		const ztd_char8_t* to_name    = (const ztd_char8_t*)u8"wide-execution";
		cnc_conversion* from_conv     = NULL;
		cnc_conversion* to_conv       = NULL;
		cnc_conversion_info from_info = {};
		cnc_conversion_info to_info   = {};
		cnc_open_error from_err
		     = cnc_conv_new_c8(registry, from_name, to_name, &from_conv, &from_info);
		cnc_open_error to_err = cnc_conv_new_c8(registry, to_name, from_name, &to_conv, &to_info);
		REQUIRE(from_err == CNC_OPEN_ERROR_OK);
		REQUIRE(to_err == CNC_OPEN_ERROR_OK);
		compare_roundtrip_utf32<ztd_wchar_t, false>(from_conv, to_conv);
		if (cnc_is_wide_execution_encoding_unicode()) {
			compare_roundtrip_utf32<ztd_wchar_t, true>(from_conv, to_conv);
		}
		cnc_conv_delete(from_conv);
		cnc_conv_delete(to_conv);
	}
	SECTION("c8") {
		const ztd_char8_t* from_name  = (const ztd_char8_t*)u8"utf32";
		const ztd_char8_t* to_name    = (const ztd_char8_t*)u8"utf8";
		cnc_conversion* from_conv     = NULL;
		cnc_conversion* to_conv       = NULL;
		cnc_conversion_info from_info = {};
		cnc_conversion_info to_info   = {};
		cnc_open_error from_err
		     = cnc_conv_new_c8(registry, from_name, to_name, &from_conv, &from_info);
		cnc_open_error to_err = cnc_conv_new_c8(registry, to_name, from_name, &to_conv, &to_info);
		REQUIRE(from_err == CNC_OPEN_ERROR_OK);
		REQUIRE(to_err == CNC_OPEN_ERROR_OK);
		compare_roundtrip_utf32<ztd_char8_t, false>(from_conv, to_conv);
		compare_roundtrip_utf32<ztd_char8_t, true>(from_conv, to_conv);
		cnc_conv_delete(from_conv);
		cnc_conv_delete(to_conv);
	}
	SECTION("c16") {
		const ztd_char8_t* from_name  = (const ztd_char8_t*)u8"utf32";
		const ztd_char8_t* to_name    = (const ztd_char8_t*)u8"utf16";
		cnc_conversion* from_conv     = NULL;
		cnc_conversion* to_conv       = NULL;
		cnc_conversion_info from_info = {};
		cnc_conversion_info to_info   = {};
		cnc_open_error from_err
		     = cnc_conv_new_c8(registry, from_name, to_name, &from_conv, &from_info);
		cnc_open_error to_err = cnc_conv_new_c8(registry, to_name, from_name, &to_conv, &to_info);
		REQUIRE(from_err == CNC_OPEN_ERROR_OK);
		REQUIRE(to_err == CNC_OPEN_ERROR_OK);
		compare_roundtrip_utf32<ztd_char16_t, false>(from_conv, to_conv);
		compare_roundtrip_utf32<ztd_char16_t, true>(from_conv, to_conv);
		cnc_conv_delete(from_conv);
		cnc_conv_delete(to_conv);
	}
	SECTION("c32") {
		const ztd_char8_t* from_name  = (const ztd_char8_t*)u8"utf32";
		const ztd_char8_t* to_name    = (const ztd_char8_t*)u8"utf32";
		cnc_conversion* from_conv     = NULL;
		cnc_conversion* to_conv       = NULL;
		cnc_conversion_info from_info = {};
		cnc_conversion_info to_info   = {};
		cnc_open_error from_err
		     = cnc_conv_new_c8(registry, from_name, to_name, &from_conv, &from_info);
		cnc_open_error to_err = cnc_conv_new_c8(registry, to_name, from_name, &to_conv, &to_info);
		REQUIRE(from_err == CNC_OPEN_ERROR_OK);
		REQUIRE(to_err == CNC_OPEN_ERROR_OK);
		compare_roundtrip_utf32<ztd_char32_t, false>(from_conv, to_conv);
		compare_roundtrip_utf32<ztd_char32_t, true>(from_conv, to_conv);
		cnc_conv_delete(from_conv);
		cnc_conv_delete(to_conv);
	}
	cnc_registry_delete(registry);
}
